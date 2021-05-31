#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class ray {
public:
	glm::vec3 o;
	glm::vec3 d;
	float tMin = 0.1f;
	float tMax = 1000.0f;

	ray(glm::vec3 o, glm::vec3 d) {
		this->o = o;
		this->d = d;
	}
};

class intersection {
private:

public:

};

class mesh {
private:
public:
	std::vector<glm::vec3> p;
	std::vector<glm::vec3> n;
	std::vector<glm::uvec3> p_indices;
	//std::vector<glm::uvec3> n_indices;

	mesh() {}

	bool rayIntersection_slow(const ray& ray) {
		bool hit = false;
		for (int i = 0; i < p_indices.size(); i++) {
			glm::vec3 p1 = p[p_indices[i].x];
			glm::vec3 p2 = p[p_indices[i].y];
			glm::vec3 p3 = p[p_indices[i].z];
			glm::vec3 normal = glm::cross(p1, p2);

			glm::vec3 p_o = ray.o + ray.d * ray.tMin;
			glm::vec3 p_e = ray.o + ray.d * ray.tMax;
			float check_o = normal.x * (p_o.x - p3.x) + normal.y * (p_o.y - p3.y) + normal.z * (p_o.z - p3.z);
			float check_e = normal.x * (p_e.x - p3.x) + normal.y * (p_e.y - p3.y) + normal.z * (p_e.z - p3.z);
			if (check_e * check_o > 0) continue;

			glm::vec3 deltaP = p3 - ray.o;
			float t = glm::dot(normal, deltaP) / glm::dot(normal, ray.d);
			glm::vec3 p_hit = ray.o + ray.d * t;

			glm::vec3 PA = p1 - p_hit;
			glm::vec3 PB = p2 - p_hit;
			glm::vec3 PC = p3 - p_hit;
			glm::vec3 CA = glm::cross(PA, PB);
			glm::vec3 CB = glm::cross(PB, PC);
			glm::vec3 CC = glm::cross(PC, PA);
			if (glm::dot(CA, CB) >= 0 && glm::dot(CB, CC) >= 0) {
				hit = true;
				break;
			}
		}
		return hit;
	}
};

class model {
public :
	std::vector<mesh> meshes;
	glm::mat4 M;
	std::string objName;

	bool rayIntersection_slow(const ray& ray) {
		bool hit = false;
		for (int i = 0; i < meshes.size(); i++) {
			if (meshes[i].rayIntersection_slow(ray)) {
				hit = true;
				break;
			}
		}
		return hit;
	}
	
	static mesh processMesh(aiMesh* aimesh, const aiScene* scene, glm::mat4 modelMat) {
		std::ofstream outputFile;
		if (DEBUG) {
			outputFile.open("MESH_DEBUG.txt");
			std::cout << "IN MESH : " << aimesh->mNumFaces << std::endl;
		}

		mesh res;
		for (int i = 0; i < aimesh->mNumVertices; i++) {
			res.p.push_back(modelMat * glm::vec4(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z, 0.0));
			glm::vec3 normal = glm::normalize(glm::vec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z));
			res.n.push_back(normal);
			if (DEBUG) {
				outputFile << "v" << " " << aimesh->mVertices[i].x / 100 << " " << aimesh->mVertices[i].y / 100 << " " << aimesh->mVertices[i].z / 100 << std::endl;
				outputFile << "vn" << " " << normal.x << " " << normal.y << " " << normal.z << std::endl;
			}
		}
		for (int i = 0; i < aimesh->mNumFaces; i++) {
			res.p_indices.push_back(glm::vec3(aimesh->mFaces[i].mIndices[0], aimesh->mFaces[i].mIndices[1], aimesh->mFaces[i].mIndices[2]));
			outputFile << "f" << 
				" " << aimesh->mFaces[i].mIndices[0] + 1 << "//" << aimesh->mFaces[i].mIndices[0] + 1 <<
				" " << aimesh->mFaces[i].mIndices[1] + 1 << "//" << aimesh->mFaces[i].mIndices[1] + 1 <<
				" " << aimesh->mFaces[i].mIndices[2] + 1 << "//" << aimesh->mFaces[i].mIndices[2] + 1 << std::endl;
		}

		if (DEBUG) {
			std::cout << "OUT MESH" << std::endl;
		}
		return res;
	}
	static void processNode(model &res, aiNode* node, const aiScene* scene) {
		if (DEBUG) {
			std::cout << "IN NODE" << node->mNumMeshes << std::endl;
		}

		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			res.meshes.push_back(processMesh(mesh, scene, res.M));
		}
		// 接下来对它的子节点重复这一过程
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(res, node->mChildren[i], scene);
		}
		if (DEBUG) {
			std::cout << "OUT NODE" << res.meshes.size() << std::endl;
		}
	}
	static model loadModel(std::string fileName, glm::mat4 modelMat) {
		model res;
		//Load mesh data
		Assimp::Importer importer;
		//const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
		const aiScene* scene = importer.ReadFile(fileName, aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
		//const aiScene* scene = importer.ReadFile(fileName, aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "ERROR : ASSIMP::No File " << fileName << std::endl;
			return res;
		}
		res.M = modelMat;
		std::cout << "LOAD MODEL" << std::endl;
		processNode(res, scene->mRootNode, scene);
		//
		res.objName = fileName.substr(0, fileName.find_last_of(".")-1);
		int tempPos = res.objName.find_last_of('\\');
		if (tempPos != -1) res.objName = fileName.substr(tempPos + 1, res.objName.length() - tempPos);
		tempPos = res.objName.find_last_of('/');
		if (tempPos != -1) res.objName = fileName.substr(tempPos + 1, res.objName.length() - tempPos);
		return res;
	}
};