#pragma once

#include "mesh.h"
#include <fstream>

class ray {
private:
	glm::vec3 o;
	glm::vec3 d;
public:

};

class intersection {
private:

public:

};

class Scene {
private:
	int prtIsV;
	int sampleNum;
	int SHNum;

public:
	std::string envMap;
	std::string sceneName;
	std::vector<mesh> world;

	Scene() {};

	bool rayIntersection_slow(const ray& ray) {
		//for (mesh in world) {}
	}

	inline int getSHNum() { return SHNum; }
	inline int getSHCoefNum() { return (SHNum + 1) * (SHNum + 1); }
	inline int getSampleNum() { return sampleNum; }

	//read scene file
	static std::shared_ptr<Scene> loadScene(std::string fileName) {
		std::shared_ptr<Scene> res = std::make_shared<Scene>();

		std::ifstream file(fileName);
		std::string line;
		//PRT -> 0:unshadowed 1:shadowed
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->prtIsV = line.at(0) - '0';
		//SH
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->SHNum = std::stoi(line);
		//sampleNum
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->sampleNum = std::stoi(line);
		//envMap
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->envMap = line;

		res->world.clear();
		//mesh
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		std::string objFile = line;

		glm::mat4 modelMat = glm::identity<glm::mat4>();
		glm::vec3 translate;
		file >> line; while (line[0] == '#') file >> line; translate[0] = std::stof(line); file >> line; translate[1] = std::stof(line); file >> line; translate[2] = std::stof(line);
		//glm::vec3 rotate;
		//file >> line; while (line[0] == '#') file >> line; rotate[0] = std::stof(line); file >> line; rotate[1] = std::stof(line); file >> line; rotate[2] = std::stof(line);
		glm::vec3 scale;
		file >> line; while (line[0] == '#') file >> line; scale[0] = std::stof(line); file >> line; scale[1] = std::stof(line); file >> line; scale[2] = std::stof(line);
		modelMat = glm::translate(modelMat, translate); modelMat = glm::scale(modelMat,scale);

		res->world.push_back(mesh::loadMesh(objFile, modelMat));

		res->sceneName = fileName.substr(0, fileName.find("config")-1);

		return res;
	}
};