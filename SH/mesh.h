#pragma once

class mesh {
private:
public:
	std::string objName;
	std::vector<glm::vec3> p;
	std::vector<glm::vec3> n;
	std::vector<glm::uvec3> p_indices;
	std::vector<glm::uvec3> n_indices;
	glm::mat4 M;

	mesh() {}

	static mesh loadMesh(std::string fileName,glm::mat4 modelMat) {
		std::ifstream objFile(fileName);

		if (!objFile.is_open()) {
			std::cout << "ERROR : No file " + fileName; 
			getchar();
			std::exit(-1);
		}

		mesh res;
		std::string attr;

		while (!objFile.eof()) {
			objFile >> attr;

			if (attr == "v") {
				glm::vec3 p;
				objFile >> attr; p[0] = std::stof(attr);
				objFile >> attr; p[1] = std::stof(attr);
				objFile >> attr; p[2] = std::stof(attr);
				res.p.push_back(p);
			}
			else if (attr == "vn") {
				glm::vec3 n;
				objFile >> attr; n[0] = std::stof(attr);
				objFile >> attr; n[1] = std::stof(attr);
				objFile >> attr; n[2] = std::stof(attr); n /= n.length();
				res.n.push_back(n);
			}
			else if (attr == "f") {
				glm::vec3 p_indices;
				glm::vec3 n_indices;
				int pos = 0;
				objFile >> attr; p_indices[0] = std::stof(attr.substr(0, attr.find('/'))) - 1; pos = attr.find_last_of('/') + 1; n_indices[0] = std::stof(attr.substr(pos, attr.length() - pos)) - 1;
				objFile >> attr; p_indices[1] = std::stof(attr.substr(0, attr.find('/'))) - 1; pos = attr.find_last_of('/') + 1; n_indices[1] = std::stof(attr.substr(pos, attr.length() - pos)) - 1;
				objFile >> attr; p_indices[2] = std::stof(attr.substr(0, attr.find('/'))) - 1; pos = attr.find_last_of('/') + 1; n_indices[2] = std::stof(attr.substr(pos, attr.length() - pos)) - 1;
				res.p_indices.push_back(p_indices);
				res.n_indices.push_back(n_indices);
			}
		}
		res.objName = fileName.substr(0, fileName.find(".obj") - 1);
		int tempPos = res.objName.find_last_of('\\'); 
		if (tempPos != -1) res.objName = fileName.substr(tempPos + 1, res.objName.length() - tempPos);
		tempPos = res.objName.find_last_of('/');
		if (tempPos != -1) res.objName = fileName.substr(tempPos + 1, res.objName.length() - tempPos);

		res.M = modelMat;

		return res;
	}
};