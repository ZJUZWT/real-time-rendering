#pragma once
#include <iomanip>

const int W = 1280;
const int H = 768;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)W / (float)H, 0.1f, 5000.0f);

GLuint areaLightProgram = 0;
GLuint meshProgram = 0;
GLuint shaderProgram = 0;
GLuint shaderCubeProgram = 0;

std::vector<std::vector<GLuint>> loadMesh(std::vector<model>);
void initShader();
void renderModel(std::vector<model>, glm::mat4 V, glm::mat4 P, std::vector<std::vector<GLuint>>, std::vector<std::vector<GLuint>>, std::vector<std::vector<GLuint>>);
GLuint cubeHDR(GLuint envHDRHandle, int sizeW, int sizeH);
GLuint loadEnvLightCoef(std::string sceneName, int coefNum);
std::vector<glm::vec3> loadEnvLightZHFCoef(std::string sceneName, int coefNum);
std::vector<glm::vec3> loadEnvSampledOmega(std::string sceneName, int coefNum);
std::vector<std::vector<GLuint>> loadPRTCoef(std::string sceneName, std::vector<model>& world, int coefNum);
std::vector<std::vector<GLuint>> loadAreaLightCoef(std::string sceneName, std::vector<model>& world, int coefNum);
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 M, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma = true);
void renderHDREnvOnCube(GLuint envHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader);
std::vector<GLuint> precomputePretabulatedMap(std::string sceneName, int SHOrder, int coefNum);
std::vector<std::vector<GLuint>> precomputeAreaLightCOEF(std::shared_ptr<Scene> worldScene, std::vector<std::vector<GLuint>> VAO);

void renderBallSet(int size, GLuint shader);

const int cubeSize = 1024;
glm::vec3 color(0);

int realTimeRender(const std::shared_ptr<Scene> &worldScene) {
	glfwInit();

	//开辟窗口
	GLFWwindow* window = glfwCreateWindow(W, H, "SH", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	initShader();
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_TEXTURE_1D);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	//预计算
	std::cout << "LOAD HDR..."; GLuint envHDRHandle = loadHDR(worldScene->envMap); std::cout << "OK" << std::endl;
	std::cout << "LOAD CUBE HDR..."; GLuint envHDRCubeHandle = cubeHDR(envHDRHandle, cubeSize, cubeSize); std::cout << "OK" << std::endl;
	std::cout << "LOAD LIGHT COEF..."; GLuint gEnvLightHandle = loadEnvLightCoef(worldScene->sceneName, worldScene->getSHCoefNum()); std::cout << "OK" << std::endl;
	std::cout << "LOAD SAMPLED OMEGA..."; std::vector<glm::vec3> gEnvSampledOmega = loadEnvSampledOmega(worldScene->sceneName, worldScene->getSHOrder() * 2 + 1); std::cout << "OK" << std::endl;
	std::cout << "LOAD LIGHT ZHF..."; std::vector<glm::vec3> gEnvLightZHF = loadEnvLightZHFCoef(worldScene->sceneName, worldScene->getSHCoefNum()); std::cout << "OK" << std::endl;
	std::cout << "LOAD PRT COEF..."; std::vector<std::vector<GLuint>> gPRTArrayHandle = loadPRTCoef(worldScene->sceneName, worldScene->world, worldScene->getSHCoefNum()); std::cout << "OK" << std::endl;
	std::cout << "LOAD MESH VAO..."; std::vector<std::vector<GLuint>> meshesVAO = loadMesh(worldScene->world); std::cout << "OK" << std::endl;
	std::cout << "LOAD AREA LIGHT..."; std::vector<std::vector<GLuint>> areaLightArrayHandle = loadAreaLightCoef(worldScene->sceneName, worldScene->world, worldScene->getSHCoefNum()); std::cout << "OK" << std::endl;
	if (worldScene->getSHType() == 2) {
		std::cout << "PRECOMPUTE Pretabulated MAP..."; std::vector<GLuint> gEnvRecoverMapHandle = precomputePretabulatedMap(worldScene->sceneName, worldScene->getSHOrder(), worldScene->getSHCoefNum()); std::cout << "OK" << std::endl; //六个面
	}
	//std::cout << "PRECOMPUTE AREA LIGHT..."; std::vector<std::vector<GLuint>> areaLightArrayHandle = precomputeAreaLightCOEF(worldScene, meshesVAO); std::cout << "OK" << std::endl;

	if (DEBUG) { 
		std::cout << "Rendering Load Finished!" << std::endl;
		outputCubeImage("debug/debug", envHDRCubeHandle, cubeSize, cubeSize);
	}

	float time = glfwGetTime();
	float lastTime = glfwGetTime() - 1 / 144.0;
	std::cout << "000.0 fps";

	//rotatedMatrix = glm::rotate(rotatedMatrix, (float)PI, rotatedAxis);

	while (!glfwWindowShouldClose(window)) {
		time = glfwGetTime();
		rotateAngle += rotateSpeed * (time - lastTime);
		rotatedMatrix = glm::rotate(glm::identity<glm::mat4>(), rotateAngle, rotatedAxis);
		//output FPS
		std::cout << "\b\b\b\b\b\b\b\b\b";
		std::cout.width(3);
		std::cout.setf(std::ios::fixed);
		std::cout << std::setprecision(1) << 1.0 / (time - lastTime) << " fps";
		lastTime = glfwGetTime();
		//genZHF
		if (worldScene->getSHType() == 1) {
			int SHOrder = worldScene->getSHOrder();
			int startIndex = 0;
			std::vector<glm::vec3> rotatedSampledOmega; rotatedSampledOmega.clear();
			for (int l = 0; l < SHOrder * 2 + 1; l++) {
				glm::vec3 res(0);
				//for (int i = 0; i < 3; i++) res[i] = gEnvSampledOmega[l][i];
				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++) res[i] += rotatedMatrix[i][j] * gEnvSampledOmega[l][j];
				rotatedSampledOmega.push_back(res);

				//std::cout << res.x << " " << res.y << " " << res.z << std::endl;
			}
			//color
			float* ZHE = new float[3* (SHOrder + 1) * (SHOrder + 1)];
			SHCoef temp(SHOrder);
			for (int l = 0; l <= SHOrder; l++) {
				int tempCoefNum = l * 2 + 1;
				Eigen::MatrixXd tempMat(tempCoefNum, tempCoefNum);
				for (int i = 0; i < tempCoefNum; i++) {
					SHFSample(temp, l, rotatedSampledOmega[i].x, rotatedSampledOmega[i].y, rotatedSampledOmega[i].z);
					for (int j = 0; j < tempCoefNum; j++)
						tempMat(i, j) = temp[startIndex + j];
				}
				
				tempMat.transposeInPlace();
				for (int i = 0; i < tempCoefNum; i++) {
					glm::vec3 tempZHE(0);
					for (int c = 0; c < 3; c++) {
						for (int j = 0; j < tempCoefNum; j++) 
							tempZHE[c] += gEnvLightZHF[startIndex + j][c] * tempMat(i, j);
					}
					ZHE[(i + startIndex) * 3 + 0] = tempZHE.x;
					ZHE[(i + startIndex) * 3 + 1] = tempZHE.y;
					ZHE[(i + startIndex) * 3 + 2] = tempZHE.z;
				}
				startIndex += tempCoefNum;
			}
			glBindTexture(GL_TEXTURE_1D, gEnvLightHandle);

			glTexImage1D(
				GL_TEXTURE_1D,
				0,
				GL_R32F,
				3 * (SHOrder + 1) * (SHOrder + 1),
				0,
				GL_RED,
				GL_FLOAT,
				ZHE);

			glBindTexture(GL_TEXTURE_1D, 0);
		}

		glfwPollEvents();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(meshProgram);
		glUniformMatrix4fv(
			glGetUniformLocation(meshProgram, "V"),
			1, GL_FALSE,
			glm::value_ptr(camera.GetViewMatrix()));
		glUniformMatrix4fv(
			glGetUniformLocation(meshProgram, "P"),
			1, GL_FALSE,
			glm::value_ptr(projection));
		glUniform3fv(
			glGetUniformLocation(meshProgram, "uColor"),
			1,
			glm::value_ptr(color));
		glUniform1i(
			glGetUniformLocation(meshProgram, "coefNum"),
			worldScene->getSHCoefNum());
		glUniform1i(
			glGetUniformLocation(meshProgram, "dispNum"),
			std::min(worldScene->getSHCoefNum(), (displaySHORDER + 1)* (displaySHORDER + 1)));
		glUniform1i(
			glGetUniformLocation(meshProgram, "envCoef"),
			0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, gEnvLightHandle);

		renderModel(worldScene->world, camera.GetViewMatrix(), projection, meshesVAO, gPRTArrayHandle, areaLightArrayHandle);
		//renderBallSet(0, meshProgram);
		if (backGroundShading) renderCubeHDREnv(envHDRCubeHandle, rotatedMatrix, camera.GetViewMatrix(), projection, shaderCubeProgram);

		glUseProgram(0);
		glBindVertexArray(0);

		for (int lightID = 0; lightID < worldScene->light.size(); lightID++) {
			areaLight light = worldScene->light[lightID];

			glColor3f(light.color.x, light.color.y, light.color.z);

			glBegin(GL_POLYGON);
			for (int pointID = 0; pointID < light.num; pointID++) {
				//std::cout << pointID << " ---------------------------";
				glm::vec4 p = glm::vec4(light.p[pointID].x, light.p[pointID].y, light.p[pointID].z, 1.0f);
				p = projection * camera.GetViewMatrix() * p; p /= p.w;
				glVertex3f(p.x, p.y, p.z);
			}
			glEnd();
		}

		if (DEBUG) {
			glUseProgram(0);
			glBindVertexArray(0);

			glm::vec4 p1 = glm::vec4(0, 0, 0, 1.0f);
			glm::vec4 p2 = glm::vec4(10, 0, 0, 1);
			glm::vec4 p3 = glm::vec4(0, 10, 0, 1);
			glm::vec4 p4 = glm::vec4(0, 0, 10, 1);
			p1 = projection * camera.GetViewMatrix() * p1; p1 /= p1.w;
			p2 = projection * camera.GetViewMatrix() * p2; p2 /= p2.w;
			p3 = projection * camera.GetViewMatrix() * p3; p3 /= p3.w;
			p4 = projection * camera.GetViewMatrix() * p4; p4 /= p4.w;
			glLineWidth(100);

			glColor3f(1.0f, 0.0f, 0.0f);
			glBegin(GL_LINES);
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p2.x, p2.y, p2.z);
			glEnd();

			glColor3f(0.0f, 1.0f, 0.0f);
			glBegin(GL_LINES);
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p3.x, p3.y, p3.z);
			glEnd();

			glColor3f(0.0f, 0.0f, 1.0f);
			glBegin(GL_LINES);
			glVertex3f(p1.x, p1.y, p1.z);
			glVertex3f(p4.x, p4.y, p4.z);
			glEnd();
		}

		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}

std::vector<std::vector<GLuint>> loadMesh(std::vector<model> world) {
	std::vector<std::vector<GLuint>> res;

	for (int modelID = 0; modelID < world.size(); modelID++) {
		std::vector<GLuint> res_model;
		model tempModel = world[modelID];
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			GLuint VAO, VBO, EBO;
			//创建句柄
			glGenVertexArrays(1, &VAO);
			glCreateBuffers(1, &VBO);
			glCreateBuffers(1, &EBO);
			//开辟内存
			GLuint lenP = tempModel.meshes[meshID].p.size() * 3;
			GLfloat* p = new GLfloat[lenP / 3 * 4];
			for (int j = 0; j < lenP / 3; j++) {
				glm::vec3 temp = tempModel.meshes[meshID].p[j];
				p[j * 4 + 0] = temp[0]; p[j * 4 + 1] = temp[1]; p[j * 4 + 2] = temp[2]; p[j * 4 + 3] = j;
			}
			GLuint lenP_indices = tempModel.meshes[meshID].p_indices.size() * 3;
			GLuint* p_indices = new GLuint[lenP_indices];
			for (int j = 0; j < lenP_indices / 3; j++) {
				glm::vec3 temp = tempModel.meshes[meshID].p_indices[j];
				p_indices[j * 3 + 0] = temp[0]; p_indices[j * 3 + 1] = temp[1]; p_indices[j * 3 + 2] = temp[2];
			}

			glNamedBufferStorage(VBO, lenP / 3 * 4 * sizeof(GLfloat), p, 0);
			glNamedBufferStorage(EBO, lenP_indices * sizeof(GLfloat), p_indices, 0);
			//绑定内存
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			//数据迁移
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
			glEnableVertexAttribArray(0);

			if (DEBUG) {
				//std::cout << "Mesh Data Output...";
				std::ofstream debugOutFile("debugMESH.txt");
				for (int j = 0; j < lenP_indices / 3; j++) {
					debugOutFile
						<< p[p_indices[j * 3 + 0] * 4 + 0] << " " << p[p_indices[j * 3 + 0] * 4 + 1] << " " << p[p_indices[j * 3 + 0] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << "/"
						<< p[p_indices[j * 3 + 1] * 4 + 0] << " " << p[p_indices[j * 3 + 1] * 4 + 1] << " " << p[p_indices[j * 3 + 1] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << "/"
						<< p[p_indices[j * 3 + 2] * 4 + 0] << " " << p[p_indices[j * 3 + 2] * 4 + 1] << " " << p[p_indices[j * 3 + 2] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << std::endl;
				}
				debugOutFile.close();
				//std::cout << " OK!" << std::endl;
			}

			delete[] p;
			delete[] p_indices;
			res_model.push_back(VAO);
		}

		res.push_back(res_model);
	}

	return res;
}
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
	ShaderInfo meshShaders[] = {
		{GL_VERTEX_SHADER,"meshShaderV.shader"} ,
		{GL_FRAGMENT_SHADER,"meshShaderF.shader"} ,
		{GL_NONE,""} };
	meshProgram = glCreateProgram();
	for (int i = 0; meshShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(meshShaders[i]);
		glAttachShader(meshProgram, shader);
	}
	glLinkProgram(meshProgram);

	//编译、连接着色器
	ShaderInfo areaLightShaders[] = {
		{GL_VERTEX_SHADER,"areaLightCoefV.shader"} ,
		{GL_FRAGMENT_SHADER,"meshShaderF.shader"} ,
		{GL_NONE,""} };
	areaLightProgram = glCreateProgram();
	for (int i = 0; areaLightShaders[i].mode != GL_NONE; i++) {
		GLuint shader = LoadShader(areaLightShaders[i]);
		glAttachShader(areaLightProgram, shader);
	}
	glLinkProgram(areaLightProgram);
}

std::vector<glm::vec3> loadEnvSampledOmega(std::string sceneName, int coefNum) {
	std::vector<glm::vec3> res;
	std::ifstream input(sceneName + "/" + "ENV_OMEGA_coef.txt");
	for (int i = 0; i < coefNum; i++) {
		glm::vec3 temp;
		input >> temp.x >> temp.y >> temp.z;
		res.push_back(temp);
	}
	input.close();
	return res;
}
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void renderHDREnvOnCube(GLuint envHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader) {
	if (!cubeVAO) {
		static const GLfloat cubeVertices[36][8] = {
			// back face
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						 1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						-1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
						// front face
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						-1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
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
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 M, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma) {
	if (!cubeVAO) {
		static const GLfloat cubeVertices[36][8] = {
			// back face
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						 1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
						 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
						-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
						-1.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
						// front face
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
						 1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						 1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
						-1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
						-1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
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
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(M));
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
	{//MENTION
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
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
GLuint loadEnvLightCoef(std::string sceneName, int coefNum) {
	std::ifstream input(sceneName + "/" + "ENV_coef.txt");
	GLfloat* coef = new GLfloat[coefNum * 3];
	for (int i = 0; i < coefNum; i++) {
		input >> coef[i * 3 + 0]; input >> coef[i * 3 + 1]; input >> coef[i * 3 + 2];
		color += glm::vec3(coef[i * 3 + 0], coef[i * 3 + 1], coef[i * 3 + 2]);
		//std::cout << coef[i * 3 + 0] << " " << coef[i * 3 + 1] << " " << coef[i * 3 + 2] << std::endl;
	}
	input.close();

	GLuint envArray;
	glGenTextures(1, &envArray);
	glBindTexture(GL_TEXTURE_1D, envArray);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage1D(
		GL_TEXTURE_1D,
		0,
		GL_R32F,
		3 * coefNum,
		0,
		GL_RED,
		GL_FLOAT,
		coef
	);
	if (DEBUG) {
		//std::cout << "Env Data Output...";
		std::ofstream debugOutFile("debugENV.txt");
		for (int j = 0; j < coefNum; j++) {
			debugOutFile
				<< coef[j * 3 + 0] << " " << coef[j * 3 + 1] << " " << coef[j * 3 + 2] << std::endl;
		}
		glm::vec3 TEST_SH(0);
		std::cout << std::endl;
		int coefNumbegin = 0;
		int coefNumEnd = 9;
		coefNum = 4;
		//SHCoef shcoef = SHFSample(2, -1, 0, 0);
		//for (int i = coefNumbegin; i < coefNumEnd; i++) std::cout << shcoef[i] << " "; std::cout << std::endl;
		//for (int i = coefNumbegin; i < coefNumEnd; i++) TEST_SH += glm::vec3(coef[i * 3 + 0], coef[i * 3 + 1], coef[i * 3 + 2]) * shcoef[i];
		//std::cout << "(-1,0,0):" << TEST_SH.x << " " << TEST_SH.y << " " << TEST_SH.z << " " << std::endl;
		//std::cout << "(-1,0,0):" << std::endl;
		//for (int i = coefNumbegin; i < coefNumEnd; i++) std::cout << "\t" << coef[i * 3 + 0] * shcoef[i] << " " << coef[i * 3 + 1] * shcoef[i] << " " << coef[i * 3 + 2] * shcoef[i] << std::endl;
		//TEST_SH.x = TEST_SH.y = TEST_SH.z = 0;
		//shcoef = SHFSample(2, 1, 0, 0);
		//for (int i = coefNumbegin; i < coefNumEnd; i++) std::cout << shcoef[i] << " "; std::cout << std::endl;
		//for (int i = coefNumbegin; i < coefNumEnd; i++) TEST_SH += glm::vec3(coef[i * 3 + 0], coef[i * 3 + 1], coef[i * 3 + 2]) * shcoef[i];
		//std::cout << "(- 0.992278 0 0.124035):" << TEST_SH.x << " " << TEST_SH.y << " " << TEST_SH.z << " " << std::endl;
		//std::cout << "( 1,0,0):" << std::endl;
		//for (int i = coefNumbegin; i < coefNumEnd; i++) std::cout << "\t" << coef[i * 3 + 0] * shcoef[i] << " " << coef[i * 3 + 1] * shcoef[i] << " " << coef[i * 3 + 2] * shcoef[i] << std::endl;


		debugOutFile.close();
		//std::cout << " OK!" << std::endl;
	}

	delete[] coef;
	return envArray;
}
std::vector<glm::vec3> loadEnvLightZHFCoef(std::string sceneName, int coefNum) {
	std::vector<glm::vec3> res;
	std::ifstream input(sceneName + "/" + "ENV_ZHF_coef.txt");
	for (int i = 0; i < coefNum; i++) {
		glm::vec3 temp;
		input >> temp.x >> temp.y >> temp.z;
		res.push_back(temp);
	}
	input.close();
	return res;
}
std::vector<std::vector<GLuint>> loadPRTCoef(std::string sceneName, std::vector<model>& world, int coefNum) {
	std::vector<std::vector<GLuint>> res;

	for (int modelID = 0; modelID < world.size(); modelID++) {
		model tempModel = world[modelID];
		std::vector<GLuint> res_mesh;
		std::ifstream input; 
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			int pNum = tempModel.meshes[meshID].p.size();
			
			input.open(sceneName + "/" + "PRT_" + world[modelID].objName + "_MESH_" + std::to_string(meshID) + "_coef.txt");
			GLfloat* coef = new GLfloat[coefNum * pNum];
			for (int i = 0; i < pNum; i++) {
				int ID;
				input >> ID;
				for (int j = 0; j < coefNum; j++) {
					input >> coef[ID * coefNum + j];
				}
				//std::cout << coef[ID * coefNum] << std::endl;
			}
			input.close(); 

			GLuint prtArray;
			GLuint prtBuffer;
			glGenTextures(1, &prtArray);
			glGenBuffers(1, &prtBuffer);

			glBindBuffer(GL_TEXTURE_BUFFER, prtBuffer);
			glBufferData(GL_TEXTURE_BUFFER, coefNum * pNum * sizeof(float), coef, GL_STATIC_DRAW);
			glBindTexture(GL_TEXTURE_BUFFER, prtArray);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, prtBuffer);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);

			if (DEBUG) {
				//std::cout << "PRT Data Output...";
				std::ofstream debugOutFile("debugPRT.txt");
				for (int i = 0; i < pNum; i++) {
					debugOutFile << i << " ";
					for (int j = 0; j < coefNum; j++) debugOutFile << coef[i * coefNum + j] << " ";
					debugOutFile << std::endl;
				}
				debugOutFile.close();
				//std::cout << " OK!" << std::endl;
			}
			res_mesh.push_back(prtArray);
			delete[] coef;
		}
		res.push_back(res_mesh);
	}
	return res;
}
std::vector<std::vector<GLuint>> loadAreaLightCoef(std::string sceneName, std::vector<model>& world, int coefNum) {
	std::vector<std::vector<GLuint>> res;

	for (int modelID = 0; modelID < world.size(); modelID++) {
		model tempModel = world[modelID];
		std::vector<GLuint> res_mesh;
		std::ifstream input;
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			int pNum = tempModel.meshes[meshID].p.size();

			input.open(sceneName + "/" + "PRT_" + world[modelID].objName + "_AREALIGHT_" + std::to_string(meshID) + "_coef.txt");
			GLfloat* coef = new GLfloat[coefNum * pNum * 3];
			for (int i = 0; i < pNum; i++) {
				for (int j = 0; j < coefNum; j++) {
					for (int c = 0; c < 3; c++)
						input >> coef[(i * coefNum + j) * 3 + c];
				}
				//std::cout << coef[ID * coefNum] << std::endl;
			}
			input.close();

			GLuint arealightArray;
			GLuint arealightBuffer;
			glGenTextures(1, &arealightArray);
			glGenBuffers(1, &arealightBuffer);

			glBindBuffer(GL_TEXTURE_BUFFER, arealightBuffer);
			glBufferData(GL_TEXTURE_BUFFER, coefNum * pNum * 3 * sizeof(float), coef, GL_STATIC_DRAW);
			glBindTexture(GL_TEXTURE_BUFFER, arealightArray);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, arealightBuffer);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);

			if (DEBUG) {
				//std::cout << "PRT Data Output...";
				std::ofstream debugOutFile("debugAERALIGHT.txt");
				for (int i = 0; i < pNum; i++) {
					debugOutFile << i << " ";
					for (int j = 0; j < coefNum; j++) for (int c = 0; c < 3; c++) debugOutFile << coef[(i * coefNum + j) * 3 + c] << " ";
					debugOutFile << std::endl;
				}
				debugOutFile.close();
				//std::cout << " OK!" << std::endl;
			}
			res_mesh.push_back(arealightArray);
			delete[] coef;
		}
		res.push_back(res_mesh);
	}
	return res;
}
void renderModel(std::vector<model> world, glm::mat4 V, glm::mat4 P, std::vector<std::vector<GLuint>> modelVAO, std::vector<std::vector<GLuint>> prtCoefHandle,std::vector<std::vector<GLuint>> areaLightArrayHandle) {
	//std::cout << meshesVAO.size();
	for (int modelID = 0; modelID < modelVAO.size(); modelID++) {
		std::vector<mesh> tempMesh = world[modelID].meshes;
		std::vector<GLuint> meshesVAO = modelVAO[modelID];
		std::vector<GLuint> meshesPRTCoef = prtCoefHandle[modelID];
		for (int meshID = 0; meshID < meshesVAO.size(); meshID++) {
			glUniformMatrix4fv(
				glGetUniformLocation(meshProgram, "M"),
				1, GL_FALSE,
				glm::value_ptr(world[modelID].M));
			//glUniform1i(
			//	glGetUniformLocation(meshProgram, "prtCoef"),
			//	1);
			//glActiveTexture(GL_TEXTURE1);
			//glBindTexture(GL_TEXTURE_2D, meshesPRTCoef[meshID]);
			glBindImageTexture(0, meshesPRTCoef[meshID], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
			glBindImageTexture(1, areaLightArrayHandle[modelID][meshID], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

			int elementNum = tempMesh[meshID].p_indices.size() * 3;

			glBindVertexArray(meshesVAO[meshID]);
			glDrawElements(GL_TRIANGLES, elementNum, GL_UNSIGNED_INT, 0);
		}
	}
}
const int PPTMSize = 1024;
std::vector<GLuint> precomputePretabulatedMap(std::string sceneName, int SHOrder, int coefNum) {
	std::vector<GLuint> res;
	std::vector<glm::vec3> lightCoef;

	std::ifstream input(sceneName + "/" + "ENV_coef.txt");
	for (int i = 0; i < coefNum; i++) {
		glm::vec3 color;
		input >> color.x >> color.y >> color.z;
		lightCoef.push_back(color);
	}
	input.close();

	glm::vec3 faceOrigin[] = {
		glm::vec3( 1.0, 1.0, 1.0),//posx
		glm::vec3(-1.0, 1.0,-1.0),//negx
		glm::vec3(-1.0, 1.0,-1.0),//posy
		glm::vec3(-1.0,-1.0, 1.0),//negy
		glm::vec3(-1.0, 1.0, 1.0),//posz
		glm::vec3( 1.0, 1.0,-1.0),//negz
	};
	glm::vec3 faceDir[] = {
		glm::vec3( 0.0, 0.0,-2.0 / PPTMSize),glm::vec3( 0.0,-2.0 / PPTMSize, 0.0),//posx
		glm::vec3( 0.0, 0.0, 2.0 / PPTMSize),glm::vec3( 0.0,-2.0 / PPTMSize, 0.0),//negx
		glm::vec3( 2.0 / PPTMSize, 0.0, 0.0),glm::vec3( 0.0, 0.0, 2.0 / PPTMSize),//posy
		glm::vec3( 2.0 / PPTMSize, 0.0, 0.0),glm::vec3( 0.0, 0.0,-2.0 / PPTMSize),//negy
		glm::vec3( 2.0 / PPTMSize, 0.0, 0.0),glm::vec3( 0.0,-2.0 / PPTMSize, 0.0),//posz
		glm::vec3(-2.0 / PPTMSize, 0.0, 0.0),glm::vec3( 0.0,-2.0 / PPTMSize, 0.0),//negz
	};
	std::string faceName[] = { "posx" , "negx" , "posy" , "negy" , "posz" , "negz" };
	SHCoef SHC((SHOrder + 1) * (SHOrder + 1));
	for (int face = 0; face < 6; face++) {
		GLuint resHandle; 
		glGenTextures(1, &resHandle);

		GLfloat* map = new GLfloat[PPTMSize * PPTMSize * 3 * (SHOrder + 1)];
		for (int l = 0; l <= SHOrder; l++) {
			for (int v = 0; v < PPTMSize; v++)
				for (int u = 0; u < PPTMSize; u++) {
					glm::vec3 color(0);
					glm::vec3 omega = faceOrigin[face] + faceDir[face * 2 + 0] * (float)u + faceDir[face * 2 + 1] * (float)v;
					SHFSample(SHC, l, omega.x, omega.y, omega.z);
					for (int m = 0; m <= l * 2 + 1; m++) color += SHC[m] * lightCoef[m];
					map[(l * PPTMSize * PPTMSize + v * PPTMSize + u) * 3 + 0] = color[0];
					map[(l * PPTMSize * PPTMSize + v * PPTMSize + u) * 3 + 1] = color[1];
					map[(l * PPTMSize * PPTMSize + v * PPTMSize + u) * 3 + 2] = color[2];
				}
			stbi_write_hdr(("pretabulated/" + faceName[face] + "_SHORDER-" + std::to_string(l) + ".hdr").c_str(), PPTMSize, PPTMSize, 3, map + l * PPTMSize * PPTMSize * 3);
		}
		glBindTexture(GL_TEXTURE_3D, resHandle);
		glTexImage3D(
			GL_TEXTURE_3D,
			0,
			GL_RGB32F,
			PPTMSize,
			PPTMSize,
			SHOrder + 1,
			0,
			GL_RGB,
			GL_FLOAT,
			map);
		res.push_back(resHandle);
		delete[] map;
	}
	return res;
}
std::vector<std::vector<GLuint>> precomputeAreaLightCOEF(std::shared_ptr<Scene> worldScene, std::vector<std::vector<GLuint>> VAO) {
	std::vector<std::vector<GLuint>> res;
	int SHOrder = worldScene->getSHOrder();
	int coefNum = worldScene->getSHCoefNum();

	GLuint uV; GLuint uVbuffer;
	GLuint uW; GLuint uWbuffer;
	GLuint uA; GLuint uAbuffer;
	GLuint uL; GLuint uLbuffer;
	GLfloat* data;
	//uV
	data = new GLfloat[worldScene->light.size() * 3];
	for (int i = 0; i < worldScene->light.size(); i++)
		data[i * 3 + 0] = worldScene->light.at(0).p[i].x,
		data[i * 3 + 1] = worldScene->light.at(0).p[i].y,
		data[i * 3 + 2] = worldScene->light.at(0).p[i].z;

	glGenTextures(1, &uV);
	glGenBuffers(1, &uVbuffer);

	glBindBuffer(GL_TEXTURE_BUFFER, uVbuffer);
	glBufferData(GL_TEXTURE_BUFFER, worldScene->light.size() * sizeof(GLfloat), data, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, uV);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, uVbuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	delete[] data;
	//uW
	data = new GLfloat[(SHOrder * 2 + 1) * 3];
	std::ifstream input(worldScene->sceneName + "/" + "ENV_OMEGA_coef.txt");
	for (int i = 0; i < SHOrder * 2 + 1; i++) {
		glm::vec3 temp;
		input >> data[i * 3 + 0] >> data[i * 3 + 1] >> data[i * 3 + 2];
	}
	input.close();
	glGenTextures(1, &uW);
	glGenBuffers(1, &uWbuffer);

	glBindBuffer(GL_TEXTURE_BUFFER, uWbuffer);
	glBufferData(GL_TEXTURE_BUFFER, (SHOrder * 2 + 1) * 3 * sizeof(GLfloat), data, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, uW);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, uWbuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	delete[] data;
	//uA
	data = new GLfloat[(SHOrder + 1) * (SHOrder * 2 + 3) * (SHOrder * 2 + 1) / 3];//(k+1)(2k+3)(2k+1)/3
	input.open(worldScene->sceneName + "/" + "ENV_A_coef.txt");
	int index = 0;
	for (int l = 0; l <= SHOrder; l++) 
		for (int i = 0; i < SHOrder * 2 + 1; i++) {
			for (int j = 0; j < SHOrder * 2 + 1; j++)
			input >> data[index++];
		}
	input.close();
	
	for (int i = 0; i < worldScene->light.size(); i++)
		data[i * 3 + 0] = worldScene->light.at(0).p[i].x,
		data[i * 3 + 1] = worldScene->light.at(0).p[i].y,
		data[i * 3 + 2] = worldScene->light.at(0).p[i].z;

	glGenTextures(1, &uA);
	glGenBuffers(1, &uAbuffer);

	glBindBuffer(GL_TEXTURE_BUFFER, uAbuffer);
	glBufferData(GL_TEXTURE_BUFFER, worldScene->light.size() * sizeof(GLfloat), data, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, uA);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, uAbuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	delete[] data;

	//bind
	glUseProgram(areaLightProgram);
	glUniform1i(glGetUniformLocation(areaLightProgram, "SHOrder"), worldScene->getSHOrder());
	glUniform1i(glGetUniformLocation(areaLightProgram, "SHCoef"), worldScene->getSHCoefNum());
	glBindImageTexture(0, uV, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(1, uW, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	glBindImageTexture(2, uA, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
	//uL
	glUniform1i(glGetUniformLocation(areaLightProgram, "M"), worldScene->light[0].num);
	for (int modelID = 0; modelID < worldScene->world.size(); modelID++) {
		model tempModel = worldScene->world[modelID];
		std::vector<GLuint> resModel;
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			mesh tempMesh = tempModel.meshes[meshID];
			int elementNum = tempMesh.p_indices.size() * 3;
			glGenTextures(1, &uL);
			glGenBuffers(1, &uLbuffer);

			glBindBuffer(GL_TEXTURE_BUFFER, uLbuffer);
			glBufferData(GL_TEXTURE_BUFFER, coefNum * tempMesh.p.size() * sizeof(GLfloat) * 3, nullptr, GL_DYNAMIC_COPY);
			glBindTexture(GL_TEXTURE_BUFFER, uL);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, uLbuffer);

			if (DEBUG) {
				std::ofstream outputFile(worldScene->sceneName + "/" + "PRT_" + tempModel.objName + "_AREALIGHT1_" + std::to_string(meshID) + "_coef.txt");
				float* data = new float[coefNum * tempMesh.p.size() * 3];
				glGetBufferSubData(GL_TEXTURE_BUFFER, sizeof(GLfloat), coefNum * tempMesh.p.size() * sizeof(GLfloat) * 3, data);
				for (int i = 0; i < tempMesh.p.size(); i++) {
					for (int j = 0; j < coefNum; j++)outputFile << data[(i * coefNum + j) * 3 + 0] << " " << data[(i * coefNum + j) * 3 + 1] << " " << data[(i * coefNum + j) * 3 + 2] << " ";
					outputFile << std::endl;
				}
				delete[] data;
				outputFile.close();
			}

			//use shader compute uL
			glBindImageTexture(3, uL, 0, GL_FALSE, 0, GL_DYNAMIC_COPY, GL_R32F);

			glBindVertexArray(VAO[modelID][meshID]);
			//glDrawElements(GL_TRIANGLES, elementNum, GL_UNSIGNED_INT, 0);
			glDrawArrays(GL_POINTS, 0, tempMesh.p.size());

			resModel.push_back(uL);

			if (DEBUG) {
				std::ofstream outputFile(worldScene->sceneName + "/" + "PRT_" + tempModel.objName + "_AREALIGHT2_" + std::to_string(meshID) + "_coef.txt");
				float* data = new float[coefNum * tempMesh.p.size() * 3];
				glGetBufferSubData(GL_TEXTURE_BUFFER, sizeof(GLfloat), coefNum * tempMesh.p.size() * sizeof(GLfloat) * 3, data);
				for (int i = 0; i < tempMesh.p.size(); i++) {
					for (int j = 0; j < coefNum; j++) outputFile << data[(i * coefNum + j) * 3 + 0] << " " << data[(i * coefNum + j) * 3 + 1] << " " << data[(i * coefNum + j) * 3 + 2] << " ";
					outputFile << std::endl;
				}
				delete[] data;
				outputFile.close();
			}

			glBindTexture(GL_TEXTURE_BUFFER, 0);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		res.push_back(resModel);
	}
	return res;
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
void renderBallSet(int size, GLuint shader) {
	for (int i = -size; i <= size; i++)
		for (int j = -size; j <= size; j++) {
			renderSphere();
		}
}