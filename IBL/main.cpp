#include <glad/glad.h>
#include <glfw/include/GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "toolkit.h"
#include "Interaction.h"

//#define PRECOMPUTE //编译选项

const int W = 1280;
const int H = 768;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)W / (float)H, 0.1f, 500.0f);

GLuint shaderProgram = 0;
GLuint shaderCubeProgram = 0;
GLuint shaderIrradianceProgram = 0;
GLuint shaderprefilterProgram = 0;
GLuint shaderPreBrdf = 0;
GLuint shaderSphere = 0;
GLuint shaderSphereOffline = 0;

GLuint cubeHDR(GLuint envHDRHandle, int sizeW, int sizeH);
GLuint calcBrdf(int sizeW, int sizeH);
GLuint calcIrradiance(GLuint envHDRHandle, int sizeW, int sizeH);
GLuint calcprefilterEnv(GLuint envHDRHandle, int sizeW, int sizeH, int level);

void initShader();
void renderBallSet(int size, GLuint cubeMap, GLuint irradianceMap, GLuint prefilterMap, GLuint brdfLUT, GLuint shader, GLuint gamma = true);
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma = true);
void renderHDREnvOnCube(GLuint envHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader);

void renderWorkOutPic(GLuint envHDRCubeHandle, GLuint envHDRIrradianceHandle, GLuint envHDRprefilterEnvHandle, GLuint brdfHandle, GLuint shader, std::string path);

const int cubeSize = 1024;
const int irradianceSize = 128;
const int prefilterSize = 1024; const int prefilterLevel = 5;
const int brdfSize = 1024;

int main(int argc, char** argv) {
	glfwInit();

	//开辟窗口
	GLFWwindow* window = glfwCreateWindow(W, H, "IBL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	//glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	initShader();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#ifdef PRECOMPUTE
	//预计算
	GLuint envHDRHandle = loadHDR("env.hdr"); std::cout << "Load!\n";
	GLuint envHDRCubeHandle = cubeHDR(envHDRHandle, cubeSize, cubeSize); std::cout << "Cube!\n";
	GLuint envHDRIrradianceHandle = calcIrradiance(envHDRCubeHandle, irradianceSize, irradianceSize); std::cout << "Irradiance!\n";
	GLuint envHDRprefilterEnvHandle = calcprefilterEnv(envHDRCubeHandle, prefilterSize, prefilterSize, prefilterLevel); std::cout << "prefilter!\n";
	GLuint brdfHandle = calcBrdf(brdfSize, brdfSize); std::cout << "BRDF!\n";
	//输出
	outputCubeImage("cube/envCube", envHDRCubeHandle, cubeSize, cubeSize); std::cout << "OUT Cube!\n";
	outputCubeImage("irradiance/envIrradiance", envHDRIrradianceHandle, irradianceSize, irradianceSize); std::cout << "OUT Irradiance!\n";
	outputCubeImage("prefilter/envPrefilter", envHDRprefilterEnvHandle, prefilterSize, prefilterSize, prefilterLevel); std::cout << "OUT prefilter!\n";
	outputImage2D("brdf/brdf", brdfHandle, brdfSize, brdfSize); std::cout << "OUT BRDF!\n";
#else
	//载入预计算贴图
	GLuint envHDRCubeHandle = loadCubemap("cube/envCube", cubeSize, cubeSize);
	GLuint envHDRIrradianceHandle = loadCubemap("irradiance/envIrradiance", irradianceSize, irradianceSize);
	GLuint envHDRprefilterEnvHandle = loadCubemap("prefilter/envPrefilter", prefilterSize, prefilterSize, prefilterLevel);
	GLuint brdfHandle = loadHDR("brdf/brdf_0.hdr", false);

	//renderWorkOutPic(envHDRCubeHandle, envHDRIrradianceHandle, envHDRprefilterEnvHandle, brdfHandle, shaderSphereOffline, "result/ref"); std::cout << "OUT offline pic\n";
	//renderWorkOutPic(envHDRCubeHandle, envHDRIrradianceHandle, envHDRprefilterEnvHandle, brdfHandle, shaderSphere, "result/result"); std::cout << "OUT real time pic\n";

	//return 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderBallSet(5, envHDRCubeHandle, envHDRIrradianceHandle, envHDRprefilterEnvHandle, brdfHandle, shaderSphere);
		renderCubeHDREnv(envHDRCubeHandle, camera.GetViewMatrix(), projection, shaderCubeProgram);

		glfwSwapBuffers(window);
    }
#endif // PRECOMPUTE
	glfwDestroyWindow(window);
	glfwTerminate();
}
GLuint cubeHDR(GLuint envHDRHandle, int sizeW, int sizeH) {
	GLuint captureFBO, captureRBO;
	//创建缓冲
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	//绑定缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	//分配内存与绑定帧缓冲
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sizeW, sizeH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	//创建立方体贴图
	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F,
			sizeW, sizeH, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//渲染立方体贴图
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, envHDRHandle);

	glViewport(0, 0, sizeW, sizeH); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; i++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderHDREnvOnCube(envHDRHandle, captureViews[i], captureProjection, shaderProgram); // renders a 1x1 cube
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W, H);
	return envCubemap;
}
GLuint calcBrdf(int sizeW, int sizeH) {
	GLuint captureFBO, captureRBO;
	//创建缓冲
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	//绑定缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	//分配内存与绑定帧缓冲
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sizeW, sizeH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	//创建BRDF贴图
	unsigned int BRDF;
	glGenTextures(1, &BRDF);
	glBindTexture(GL_TEXTURE_2D, BRDF);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, sizeW, sizeH, 0, GL_RGB, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//绑定FBO
	glViewport(0, 0, sizeW, sizeH);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BRDF, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const float vertices[] = {
		1.0,1.0,0.0,
		1.0,-1.0,0.0,
		-1.0,-1.0,0.0,
		1.0,1.0,0.0,
		-1.0,1.0,0.0,
		-1.0,-1.0,0.0 };

	GLuint planeVAO, planeVBO;
	glGenVertexArrays(1,&planeVAO);
	glCreateBuffers(1, &planeVBO);

	glNamedBufferStorage(planeVBO, sizeof(vertices), vertices, 0);
	//绑定内存
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	//数据迁移
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	//计算
	glUseProgram(shaderPreBrdf);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//恢复
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W, H);
	return BRDF;
}
GLuint calcIrradiance(GLuint envHDRHandle, int sizeW, int sizeH) {
	GLuint captureFBO, captureRBO;
	//创建缓冲
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	//绑定缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	//分配内存与绑定帧缓冲
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sizeW, sizeH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	//创建立方体贴图
	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F,
			sizeW, sizeH, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//渲染立方体贴图
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, envHDRHandle);

	glViewport(0, 0, sizeW, sizeH); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; i++) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCubeHDREnv(envHDRHandle, captureViews[i], captureProjection, shaderIrradianceProgram); // renders a 1x1 cube
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W, H);
	return envCubemap;
}
GLuint calcprefilterEnv(GLuint envHDRHandle, int sizeW, int sizeH, int level) {
	GLuint captureFBO, captureRBO;
	//创建缓冲
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	//绑定缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	//分配内存与绑定帧缓冲
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, sizeW, sizeH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	//创建立方体贴图
	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F,
			sizeW, sizeH, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);//构造MipMap
	//渲染立方体贴图
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, envHDRHandle);

	for (unsigned int mip = 0, miplevel = 1; mip < level; mip++, miplevel *= 2) {
		glViewport(0, 0, sizeW / miplevel, sizeH / miplevel); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(shaderprefilterProgram);
			float roughness = (float)mip / (float)(level - 1);
			glUniform1f(glGetUniformLocation(shaderprefilterProgram, "roughness"), roughness);
			renderCubeHDREnv(envHDRHandle, captureViews[i], captureProjection, shaderprefilterProgram); // renders a 1x1 cube
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W, H);
	return envCubemap;
}
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void initShader() {
	//编译、连接着色器
	ShaderInfo Shaders[] = {
		{GL_VERTEX_SHADER,"envShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"envShaderF.shader"} ,
		{GL_NONE,""} };
	shaderProgram = glCreateProgram();
	for (int i = 0; Shaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(Shaders[i]);
		glAttachShader(shaderProgram, shader);
	}
	glLinkProgram(shaderProgram);

	//编译、连接着色器
	ShaderInfo cubeShaders[] = {
		{GL_VERTEX_SHADER,"envCubeShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"envCubeShaderF.shader"} ,
		{GL_NONE,""} };
	shaderCubeProgram = glCreateProgram();
	for (int i = 0; cubeShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(cubeShaders[i]);
		glAttachShader(shaderCubeProgram, shader);
	}
	glLinkProgram(shaderCubeProgram);

	//编译、连接着色器
	ShaderInfo irradianceShaders[] = {
		{GL_VERTEX_SHADER,"envIrradianceShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"envIrradianceShaderF.shader"} ,
		{GL_NONE,""} };
	shaderIrradianceProgram = glCreateProgram();
	for (int i = 0; irradianceShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(irradianceShaders[i]);
		glAttachShader(shaderIrradianceProgram, shader);
	}
	glLinkProgram(shaderIrradianceProgram);

	//编译、连接着色器
	ShaderInfo prefilterShaders[] = {
		{GL_VERTEX_SHADER,"envprefilterShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"envprefilterShaderF.shader"} ,
		{GL_NONE,""} };
	shaderprefilterProgram = glCreateProgram();
	for (int i = 0; prefilterShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(prefilterShaders[i]);
		glAttachShader(shaderprefilterProgram, shader);
	}
	glLinkProgram(shaderprefilterProgram);

	//编译、连接着色器
	ShaderInfo preBrdfShaders[] = {
		{GL_VERTEX_SHADER,"preBrdfShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"preBrdfShaderF.shader"} ,
		{GL_NONE,""} };
	shaderPreBrdf = glCreateProgram();
	for (int i = 0; preBrdfShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(preBrdfShaders[i]);
		glAttachShader(shaderPreBrdf, shader);
	}
	glLinkProgram(shaderPreBrdf);

	//编译、连接着色器
	ShaderInfo sphereShaders[] = {
		{GL_VERTEX_SHADER,"sphereShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"sphereShaderF.shader"} ,
		{GL_NONE,""} };
	shaderSphere = glCreateProgram();
	for (int i = 0; sphereShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(sphereShaders[i]);
		glAttachShader(shaderSphere, shader);
	}
	glLinkProgram(shaderSphere);

	//编译、连接着色器
	ShaderInfo sphereOfflineShaders[] = {
		{GL_VERTEX_SHADER,"sphereShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"sphereOfflineShaderF.shader"} ,
		{GL_NONE,""} };
	shaderSphereOffline = glCreateProgram();
	for (int i = 0; sphereOfflineShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(sphereOfflineShaders[i]);
		glAttachShader(shaderSphereOffline, shader);
	}
	glLinkProgram(shaderSphereOffline);
}
void renderHDREnvOnCube(GLuint envHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader) {
	if (!cubeVAO) {
		static const GLfloat cubeVertices[36][8] = {
			// back face
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
						// front face
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						// left face
						-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
						-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
						-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
						-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
						-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
						-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
						// right face
						 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
						 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
						 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
						 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
						 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
						// bottom face
						-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
						 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
						 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
						 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
						-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
						-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
						// top face
						-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
						 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
						 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
						-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
						-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		//创建句柄
		glGenVertexArrays(1, &cubeVAO);
		glCreateBuffers(1, &cubeVBO);
		//开辟内存
		glNamedBufferStorage(cubeVBO, sizeof(cubeVertices), cubeVertices, 0);
		//绑定内存
		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		//数据迁移
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
	}
	GLuint uniformLocation;
	glUseProgram(shader);

	uniformLocation = glGetUniformLocation(shader, "M");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
	uniformLocation = glGetUniformLocation(shader, "V");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(V));
	uniformLocation = glGetUniformLocation(shader, "P");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(P));
	uniformLocation = glGetUniformLocation(shader, "hdrSampler");
	glUniform1i(uniformLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, envHDRHandle);

	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUseProgram(0);
}
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma) {
	if (!cubeVAO) {
		static const GLfloat cubeVertices[36][8] = {
			// back face
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
						// front face
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						// left face
						-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
						-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
						-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
						-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
						-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
						-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
						// right face
						 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
						 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
						 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
						 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
						 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
						// bottom face
						-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
						 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
						 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
						 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
						-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
						-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
						// top face
						-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
						 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
						 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
						-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
						-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		//创建句柄
		glGenVertexArrays(1, &cubeVAO);
		glCreateBuffers(1, &cubeVBO);
		//开辟内存
		glNamedBufferStorage(cubeVBO, sizeof(cubeVertices), cubeVertices, 0);
		//绑定内存
		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		//数据迁移
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);
	}
	GLuint uniformLocation;
	glUseProgram(shader);

	uniformLocation = glGetUniformLocation(shader, "M");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
	uniformLocation = glGetUniformLocation(shader, "V");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(V));
	uniformLocation = glGetUniformLocation(shader, "P");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(P));
	uniformLocation = glGetUniformLocation(shader, "isGamma");
	glUniform1i(uniformLocation, gamma);
	uniformLocation = glGetUniformLocation(shader, "hdrSampler");
	glUniform1i(uniformLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubeHDRHandle);

	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUseProgram(0);
}
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = indices.size();

		std::vector<float> data;
		for (std::size_t i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}
void renderBallSet(int size, GLuint cubeMap, GLuint irradianceMap, GLuint prefilterMap, GLuint brdfLUT, GLuint shader, GLuint gamma) {
	glUseProgram(shader);
	glUniformMatrix4fv(
		glGetUniformLocation(shader, "V"),
		1, GL_FALSE,
		glm::value_ptr(camera.GetViewMatrix()));
	glUniformMatrix4fv(
		glGetUniformLocation(shader, "P"),
		1, GL_FALSE,
		glm::value_ptr(projection));
	glUniform3fv(
		glGetUniformLocation(shader, "camPos"),
		1,
		glm::value_ptr(camera.GetViewPosition()));
	glUniform3fv(
		glGetUniformLocation(shader, "albedo"),
		1,
		glm::value_ptr(glm::vec3(1.0, 1.0, 1.0)));
	glUniform1i(
		glGetUniformLocation(shader, "isGamma"),
		gamma);
	glUniform1i(
		glGetUniformLocation(shader, "irradianceMap"),
		0);
	glUniform1i(
		glGetUniformLocation(shader, "prefilterMap"),
		1);
	glUniform1i(
		glGetUniformLocation(shader, "brdfLUT"),
		2);
	glUniform1i(
		glGetUniformLocation(shader, "hdrSampler"),
		3);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	for (int i = -size; i <= size; i++)
		for (int j = -size; j <= size; j++) {
			glUniform1f(
				glGetUniformLocation(shader, "roughness"),
				1.0f * (i + size) / (size * 2 + 1));
			glUniform1f(
				glGetUniformLocation(shader, "metalness"),
				1.0f * (j + size) / (size * 2 + 1));
			glUniformMatrix4fv(
				glGetUniformLocation(shader, "M"),
				1, GL_FALSE,
				glm::value_ptr(glm::translate(glm::mat4(1.0), glm::vec3(i * 3, j * 3, 0))));

			renderSphere();
		}
}

void renderWorkOutPic(GLuint envHDRCubeHandle, GLuint envHDRIrradianceHandle, GLuint envHDRprefilterEnvHandle, GLuint brdfHandle, GLuint shader, std::string path) {
	const int picW = W * 4, picH = H * 4;

	GLuint captureFBO, captureRBO;
	//创建缓冲
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	//绑定缓冲
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	//分配内存与绑定帧缓冲
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, picW, picH);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
	//创建纹理缓存
	unsigned int captureTex;
	glGenTextures(1, &captureTex);
	glBindTexture(GL_TEXTURE_2D, captureTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, picW, picH, 0, GL_RGB, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//绑定FBO
	glViewport(0, 0, picW, picH);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, captureTex, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderBallSet(5, envHDRCubeHandle, envHDRIrradianceHandle, envHDRprefilterEnvHandle, brdfHandle, shader, false);
	renderCubeHDREnv(envHDRCubeHandle, camera.GetViewMatrix(), projection, shaderCubeProgram, false);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W, H);
	outputImage2D(path.c_str(), captureTex, picW, picH, true);
}