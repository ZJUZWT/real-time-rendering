#pragma once

#include<vector>
#include<string>
#include<sstream>
#include<fstream>
#include<iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "OpenGL/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "OpenGL/stb_image_write.h"
//---------------------------------------------------------------------------------SHADER
struct ShaderInfo {
	int mode;
	std::string fileName;
};
GLuint LoadShader(ShaderInfo shaders) {
	GLuint shader = glCreateShader(shaders.mode);

	std::string shaderCode;
	std::ifstream shaderFile;

	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		shaderFile.open(shaders.fileName);

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();
		shaderCode = shaderStream.str();
	}
	catch (std::ifstream::failure e) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
	}

	int success;
	char infoLog[512];

	const char* shaderSource = shaderCode.c_str();
	glShaderSource(shader, 1, &shaderSource, nullptr);

	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cout << shaders.fileName + "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	return shader;
}
//---------------------------------------------------------------------------------TEXTURE
enum { AmbientTexture, NormalTexture, RoughnessTexture , Texture_NONE};
struct TextureInfo {
	int mode;
	std::string fileName;
};

//---------------------------------------------------------------------------------CUBE BOX
std::vector<std::string> faces
{
	"skybox/right.jpg",
	"skybox/left.jpg",
	"skybox/top.jpg",
	"skybox/bottom.jpg",
	"skybox/front.jpg",
	"skybox/back.jpg"
};
GLuint loadCubemap(std::vector<std::string> faces){
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
GLuint loadCubemap(std::string path, int w, int h, int miplevel = 1) {
	GLuint textureID;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	glTextureStorage2D(textureID, miplevel, GL_RGB32F, w, h);

	float* data;
	int width, height, nrChannels;
	for (int i = 0; i < miplevel; i++) {
		data = stbi_loadf((path + "_" + std::to_string(i) + "_X_POSITIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 0, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_X_POSITIVE.hdr" << std::endl; stbi_image_free(data); }
		data = stbi_loadf((path + "_" + std::to_string(i) + "_X_NEGATIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 1, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_X_NEGATIVE.hdr" << std::endl; stbi_image_free(data); }
		data = stbi_loadf((path + "_" + std::to_string(i) + "_Y_POSITIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 2, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_Y_POSITIVE.hdr" << std::endl; stbi_image_free(data); }
		data = stbi_loadf((path + "_" + std::to_string(i) + "_Y_NEGATIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 3, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_Y_NEGATIVE.hdr" << std::endl; stbi_image_free(data); }
		data = stbi_loadf((path + "_" + std::to_string(i) + "_Z_POSITIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 4, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_Z_POSITIVE.hdr" << std::endl; stbi_image_free(data); }
		data = stbi_loadf((path + "_" + std::to_string(i) + "_Z_NEGATIVE.hdr").c_str(), &width, &height, &nrChannels, 0);
		if (data) glTextureSubImage3D(textureID, i, 0, 0, 5, width, height, 1, GL_RGB, GL_FLOAT, data);
		else { std::cout << "Cubemap texture failed to load at path: " << path + "_" + std::to_string(i) + "_Z_NEGATIVE.hdr" << std::endl; stbi_image_free(data); }
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (miplevel != 1)
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	return textureID;
}
GLuint loadHDR(std::string path, bool flip = true) {//HDR文件会保存浮点数，这个和其他的文件就不一样的，其精度高的多，并且读取到的是线性空间的数值
	GLuint textureID;
	//生成句柄
	glGenTextures(1, &textureID);
	//绑定句柄信息
	glBindTexture(GL_TEXTURE_2D, textureID);

	int width, height, nrChannels;
	//读取图片
	stbi_set_flip_vertically_on_load(flip);
	float* data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		//填充内容给当前绑定的句柄
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);
		stbi_image_free(data);
	} else {
		std::cout << "HDR texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;
}

void outputImage2D(std::string name, GLuint handle, size_t width, size_t height, int mipmap = 0) {
	float* map = new float[height * width * 3];
	glBindTexture(GL_TEXTURE_2D, handle);
	for (int i = 0, miplevel = 1; i <= mipmap; i++, miplevel *= 2) {
		glGetTexImage(GL_TEXTURE_2D, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + ".hdr").c_str(), width, height, 3, map);
		width /= 2;	height /= 2;
	}
	delete[] map;
}

void outputCubeImage(std::string name,GLuint handle, size_t width, size_t height, int mipmap = 0) {
	float* map = new float[height * width * 3];
	float* sumMap = new float[height * width * 3 * 12];
	size_t offset;
	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
	for (int i = 0, miplevel = 1; i <= mipmap; i++, miplevel *= 2) {
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + "_X_POSITIVE.hdr").c_str(), width, height, 3, map); 
		offset = (1 * height * 4 * width + width * 2) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + "_X_NEGATIVE.hdr").c_str(), width, height, 3, map);
		offset = (1 * height * 4 * width + width * 0) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, GL_RGB, GL_FLOAT, (void*)map); 
		stbi_write_hdr((name + "_" + std::to_string(i) + "_Y_POSITIVE.hdr").c_str(), width, height, 3, map); 
		offset = (0 * height * 0 * width + width * 1) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + "_Y_NEGATIVE.hdr").c_str(), width, height, 3, map); 
		offset = (2 * height * 4 * width + width * 1) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + "_Z_POSITIVE.hdr").c_str(), width, height, 3, map);
		offset = (1 * height * 4 * width + width * 1) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, GL_RGB, GL_FLOAT, (void*)map);
		stbi_write_hdr((name + "_" + std::to_string(i) + "_Z_NEGATIVE.hdr").c_str(), width, height, 3, map);
		offset = (1 * height * 4 * width + width * 3) * 3;
		for (int j = 0; j < height; j++) memcpy(sumMap + offset + width * 4 * 3 * j, map + width * j * 3, sizeof(float) * width * 3);

		stbi_write_hdr((name + "_" + std::to_string(i) + "_SUM.hdr").c_str(), width * 4, height * 3, 3, sumMap);
		memset(sumMap, 0, sizeof(sumMap));

		width /= 2;	height /= 2;
	}
	delete[] map;
	delete[] sumMap;
}