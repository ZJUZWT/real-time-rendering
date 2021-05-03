#pragma once

const int W = 1280;
const int H = 768;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)W / (float)H, 0.1f, 5000.0f);

GLuint meshProgram = 0;
GLuint shaderProgram = 0;
GLuint shaderCubeProgram = 0;

std::vector<GLuint> loadMesh(std::vector<mesh>);
void initShader();
void renderMeshes(std::vector<mesh>, glm::mat4 V, glm::mat4 P, std::vector<GLuint>, std::vector<GLuint>);
GLuint cubeHDR(GLuint envHDRHandle, int sizeW, int sizeH);
GLuint loadEnvLightCoef(std::string sceneName, int coefNum);
std::vector<GLuint> loadPRTCoef(std::string sceneName, std::vector<mesh>& world, int coefNum);
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma = true);
void renderHDREnvOnCube(GLuint envHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader);

void renderBallSet(int size, GLuint shader);

const int cubeSize = 1024;
glm::vec3 color(0);

int realTimeRender(std::shared_ptr<Scene> worldScene) {
	glfwInit();

	//开辟窗口
	GLFWwindow* window = glfwCreateWindow(W, H, "SH", nullptr, nullptr);
	glfwMakeContextCurrent(window);

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

	//预计算
	GLuint envHDRHandle = loadHDR(worldScene->envMap);
	GLuint envHDRCubeHandle = cubeHDR(envHDRHandle, cubeSize, cubeSize);
	GLuint gEnvLightHandle = loadEnvLightCoef(worldScene->sceneName, worldScene->getSHCoefNum());
	std::vector<GLuint> gPRTArrayHandle = loadPRTCoef(worldScene->sceneName, worldScene->world, worldScene->getSHCoefNum());
	std::vector<GLuint> meshesVAO = loadMesh(worldScene->world);
	
	if (DEBUG) { 
		std::cout << "Rendering Load Finished!" << std::endl;
		outputCubeImage("debug/debug", envHDRCubeHandle, cubeSize, cubeSize);
		std::cout << "Rendering Load Finished!" << std::endl;
	}

	while (!glfwWindowShouldClose(window)) {
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
			glGetUniformLocation(meshProgram, "envCoef"),
			0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, gEnvLightHandle);

		renderMeshes(worldScene->world, camera.GetViewMatrix(), projection, meshesVAO, gPRTArrayHandle);
		//renderBallSet(0, meshProgram);
		renderCubeHDREnv(envHDRCubeHandle, camera.GetViewMatrix(), projection, shaderCubeProgram);

		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}

std::vector<GLuint> loadMesh(std::vector<mesh> world) {
	std::vector<GLuint> res;

	for (int i = 0; i < world.size(); i++) {
		GLuint VAO, VBO, EBO;
		//创建句柄
		glGenVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);
		glCreateBuffers(1, &EBO);
		//开辟内存
		GLuint lenP = world[i].p.size() * 3;
		GLfloat* p = new GLfloat[lenP/3*4];
		for (int j = 0; j < lenP / 3; j++) {
			glm::vec3 temp = world[i].p[j];
			p[j * 4 + 0] = temp[0]; p[j * 4 + 1] = temp[1]; p[j * 4 + 2] = temp[2]; p[j * 4 + 3] = j;
		}
		GLuint lenP_indices = world[i].p_indices.size() * 3;
		GLuint* p_indices = new GLuint[lenP_indices];
		for (int j = 0; j < lenP_indices / 3; j++) {
			glm::vec3 temp = world[i].p_indices[j];
			p_indices[j * 3 + 0] = temp[0] ; p_indices[j * 3 + 1] = temp[1] ; p_indices[j * 3 + 2] = temp[2] ;
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
			std::cout << "Mesh Data Output..." ;
			std::ofstream debugOutFile("debugMESH.txt");
			for (int j = 0; j < lenP_indices / 3; j++) {
				debugOutFile 
					<< p[p_indices[j * 3 + 0] * 4 + 0] << " " << p[p_indices[j * 3 + 0] * 4 + 1] << " " << p[p_indices[j * 3 + 0] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << "/"
					<< p[p_indices[j * 3 + 1] * 4 + 0] << " " << p[p_indices[j * 3 + 1] * 4 + 1] << " " << p[p_indices[j * 3 + 1] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << "/"
					<< p[p_indices[j * 3 + 2] * 4 + 0] << " " << p[p_indices[j * 3 + 2] * 4 + 1] << " " << p[p_indices[j * 3 + 2] * 4 + 2] << p[p_indices[j * 3 + 0] * 4 + 3] << " " << std::endl;
			}
			debugOutFile.close();
			std::cout << " OK!" << std::endl;
		}

		delete[] p;
		delete[] p_indices;
		res.push_back(VAO);
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
void renderCubeHDREnv(GLuint envCubeHDRHandle, glm::mat4 V, glm::mat4 P, GLuint shader, GLuint gamma) {
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
	std::ifstream input(sceneName + "_ENV_coef.txt");
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
		std::cout << "Env Data Output...";
		std::ofstream debugOutFile("debugENV.txt");
		for (int j = 0; j < coefNum; j++) {
			debugOutFile
				<< coef[j * 3 + 0] << " " << coef[j * 3 + 1] << " " << coef[j * 3 + 2] << std::endl;
		}
		debugOutFile.close();
		std::cout << " OK!" << std::endl;
	}

	delete[] coef;
	return envArray;
}
std::vector<GLuint> loadPRTCoef(std::string sceneName, std::vector<mesh>& world, int coefNum) {
	std::vector<GLuint> res;

	for (int meshID = 0; meshID < world.size(); meshID++) {
		int pNum = world[meshID].p.size();

		std::ifstream input(sceneName + "_PRT_" + world[meshID].objName + "_coef.txt");
		GLfloat* coef = new GLfloat[coefNum * pNum];
		for (int i = 0; i < pNum; i++) {
			int ID;
			input >> ID;
			for (int j = 0; j < coefNum; j++) {
				input >> coef[ID * coefNum + j];
			}
		}
		input.close();

		GLuint prtArray;
		glGenTextures(1, &prtArray);
		glBindTexture(GL_TEXTURE_1D, prtArray);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage1D(
			GL_TEXTURE_1D,
			0,
			GL_R32F,
			coefNum * pNum,
			0,
			GL_RED,
			GL_FLOAT,
			coef
		);

		if (DEBUG) {
			std::cout << "PRT Data Output...";
			std::ofstream debugOutFile("debugPRT.txt");
			for (int i = 0; i < pNum; i++) {
				debugOutFile << i << " ";
				for (int j = 0; j < coefNum; j++) debugOutFile << coef[i * coefNum + j] << " ";
				debugOutFile << std::endl;
			}
			debugOutFile.close();
			std::cout << " OK!" << std::endl;
		}

		res.push_back(prtArray);
		delete[] coef;
	}
	return res;
}
void renderMeshes(std::vector<mesh> world, glm::mat4 V, glm::mat4 P, std::vector<GLuint> meshesVAO, std::vector<GLuint> prtCoefHandle) {
	//std::cout << meshesVAO.size();
	for (int i = 0; i < meshesVAO.size(); i++) {
		glUniformMatrix4fv(
			glGetUniformLocation(meshProgram, "M"),
			1, GL_FALSE,
			glm::value_ptr(world[i].M));
		glUniform1i(
			glGetUniformLocation(meshProgram, "prtCoef"),
			1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, prtCoefHandle[i]);

		int elementNum = world[i].p_indices.size() * 3;

		glBindVertexArray(meshesVAO[i]);
		glDrawElements(GL_TRIANGLES, elementNum, GL_UNSIGNED_INT, 0);
	}
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