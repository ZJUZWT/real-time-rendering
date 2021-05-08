#pragma once

#include "mesh.h"
#include <fstream>
#include <direct.h>

class Scene {
private:
	int prtIsV;
	int sampleNum;
	int SHOrder;
	int SHType;

public:
	std::string envMap;
	std::string sceneName;
	std::vector<model> world;


	Scene() {};

	bool rayIntersection_slow(const ray& ray) {
		bool hit = false;
		for (int i = 0; i < world.size(); i++) {
			if (world[i].rayIntersection_slow(ray)) {
				hit = true;
				break;
			}
		}
		return hit;
	}

	inline int getV() { return prtIsV; }
	inline int getSHOrder() { return SHOrder; }
	inline int getSHCoefNum() { return (SHOrder + 1) * (SHOrder + 1); }
	inline int getSampleNum() { return sampleNum; }
	inline int getSHType() { return SHType; }

	//read scene file
	static std::shared_ptr<Scene> loadScene(std::string fileName) {
		std::shared_ptr<Scene> res = std::make_shared<Scene>();

		std::ifstream file(fileName);
		std::string line;
		//PRT -> 0:unshadowed 1:shadowed
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->prtIsV = std::stoi(line);
		//SHType
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->SHType = std::stoi(line);
		//SHOrder
		std::getline(file, line);
		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		res->SHOrder = std::stoi(line);
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
		//while (!file.eof()) {
		std::getline(file, line);
		//if (line == "$") break;

		if (line.find('#') != -1) line = line.substr(0, line.find('#')); while (line.find(' ') != -1) line = line.substr(0, line.find(' ')); while (line.find('\t') != -1) line = line.substr(0, line.find('\t'));
		std::string objFile = line;

		glm::mat4 modelMat = glm::identity<glm::mat4>();
		glm::vec3 translate;
		file >> line; while (line[0] == '#') file >> line; translate[0] = std::stof(line); file >> line; translate[1] = std::stof(line); file >> line; translate[2] = std::stof(line);
		glm::vec3 scale;
		file >> line; while (line[0] == '#') file >> line; scale[0] = std::stof(line); file >> line; scale[1] = std::stof(line); file >> line; scale[2] = std::stof(line);
		modelMat = glm::translate(modelMat, translate); modelMat = glm::scale(modelMat, scale);

		res->world.push_back(model::loadModel(objFile, modelMat));
		//}

		res->sceneName = fileName.substr(0, fileName.find("config") - 1);

		_mkdir(res->sceneName.c_str());

		return res;
	}
};