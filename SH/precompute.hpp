#pragma once
#include "mesh.h"
#include "SHFSampler.h"
#include <random>
#include <time.h>

#include <Eigen/Dense>

glm::vec3 sphericalRandomSample() {
	std::random_device rd;
	std::default_random_engine e(rd());
	std::uniform_real_distribution<Float> ran(-1.0, 1.0);

	Float u = ran(e);
	Float v = ran(e);
	Float r2 = u * u + v * v;

	while (r2 > 1.0) {
		u = ran(e);
		v = ran(e);
		r2 = u * u + v * v;
	}
	return glm::vec3(2 * u * sqrt(1 - r2), 2 * v * sqrt(1 - r2), 1 - 2 * r2);
}

void precomputeENV(std::shared_ptr<Scene> worldScene) {
	std::ofstream outputFile;

	std::string sceneName = worldScene->sceneName;
	int SHOrder = worldScene->getSHOrder();
	int coefNum = (1 + SHOrder) * (1 + SHOrder);

	int width, height, nrChannels;
	std::string path = worldScene->envMap;
	float* data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);

	SHColor env_SH(SHOrder);
	SHCoef SHF(SHOrder);
	for (int i = 0; i < coefNum; i++)
		for (int c = 0; c < nrChannels; c++)
			env_SH[c][i] = 0;

	std::cout << "Precompute ENV :   0%";

	for (int v = 0; v < height; v++) { //ÀèÂüºÍ
		Float area = 2 * PI * (cos(v * 1.0 / (height + 1) * PI) - cos((v * 1.0 + 1) / (height + 1) * PI)) / width;

		for (int u = 0; u < width; u++) {
			Float Li[3] = {
				data[(v * width + u) * nrChannels + 0],
				data[(v * width + u) * nrChannels + 1],
				data[(v * width + u) * nrChannels + 2]};

			Float
				//y = cos(1.0 * v / height * PI),//stb vertical reverse
				//x = sqrt(1 - y * y) * cos((u * 1.0 / width - 0.5) * PI * 2),
				//z = sqrt(1 - y * y) * sin((u * 1.0 / width - 0.5) * PI * 2);
				y = cos((1.0 * v / height * PI)), //[0,HEIGHT] -> [1,-1]
				x = sqrt(1 - y * y) * cos((u * 1.0 / width) * PI * 2),
				z = sqrt(1 - y * y) * sin((u * 1.0 / width) * PI * 2);

			SHFSample(SHF, SHOrder, x, y, z);

			for (int c = 0; c < nrChannels; c++)
				for (int i = 0; i < coefNum; i++)
					env_SH[c][i] += Li[c] * SHF[i] * area;
		}
		std::cout << "\b\b\b\b";
		std::cout.width(3);
		std::cout << v*100/(height-1) << "%";
	}
	std::cout << std::endl;
	stbi_image_free(data);

	outputFile.open(sceneName + "/" + "ENV_coef.txt");
	for (int i = 0; i < coefNum; i++) {
		for (int c = 0; c < nrChannels; c++)
			outputFile << env_SH[c][i] << " ";
		outputFile << std::endl;
	}
	outputFile.close();
}
void precomputePRT(std::shared_ptr<Scene> worldScene) {
	std::ofstream outputFile;

	std::string sceneName = worldScene->sceneName;
	int SHOrder = worldScene->getSHOrder();
	int coefNum = (1 + SHOrder) * (1 + SHOrder);
	int sampleNum = worldScene->getSampleNum();

	std::vector<glm::vec3> SHEnv;
	std::ofstream debugPreOutput;
	std::ofstream debugOutputFile;
	if (DEBUG) {
		std::ifstream input(sceneName + "/" + "ENV_coef.txt");
		for (int i = 0; i < coefNum; i++) {
			glm::vec3 temp;
			input >> temp.x >> temp.y >> temp.z;
			SHEnv.push_back(temp);
		}
		input.close();
		debugOutputFile.open(sceneName + "/" + "DEBUG_PRT_res.txt");
	}

	for (int modelID = 0; modelID < worldScene->world.size(); modelID++) {
		model &tempModel = worldScene->world[modelID];
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			mesh& tempMesh = tempModel.meshes[meshID];
			unsigned int* vertexMap = new unsigned int[tempMesh.p.size()]{ 0 };
			std::cout << "Precompute PRT MODEL : " << tempModel.objName << " - MESH : " << meshID << " :   0%";
			outputFile.open(sceneName + "/" + "PRT_" + tempModel.objName + "_MESH_" + std::to_string(meshID) + "_coef.txt");
			SHCoef shCoef(SHOrder);
			SHCoef SHF(SHOrder);
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> rng(0.0, 1.0);

			for (int vertexID = 0; vertexID < tempMesh.p_indices.size() * 3; vertexID++) {
				if (!vertexMap[tempMesh.p_indices[vertexID / 3][vertexID % 3]]) { 
					glm::vec3 p = tempMesh.p[tempMesh.p_indices[vertexID / 3][vertexID % 3]];
					glm::vec3 n = tempMesh.n[tempMesh.p_indices[vertexID / 3][vertexID % 3]];
					//for (int i = 0; i < coefNum; i++) shCoef[i] = 0;
			//		//sample2 reference
					int sample_side = sqrt(sampleNum);
					for (int sample1 = 0; sample1 < sample_side; sample1++) {
						for (int sample2 = 0; sample2 < sample_side; sample2++) {
							double alpha = (sample1 + rng(gen)) / sample_side;
							double beta = (sample2 + rng(gen)) / sample_side;

							double phi = 2.0 * PI * beta;
							double theta = acos(2.0 * alpha - 1.0);

							glm::vec3 wi(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
							Float NdotW = glm::dot(n, wi);
							if (NdotW <= 0) continue;

							double func_value = NdotW;
							if (worldScene->getV() != 0) {
								ray ray(p, wi);
								func_value *= 1 - worldScene->rayIntersection_slow(ray);
							}

							SHFSample(SHF, SHOrder, wi[0], wi[1], wi[2]);
							SHFSample(SHF, SHOrder, n.x, n.y, n.z);

							for (int i = 0; i < coefNum; i++) shCoef[i] += SHF[i] * func_value;

							//if (tempMesh.p_indices[vertexID / 3][vertexID % 3] > 5000)
								//std::cout << tempMesh.p_indices[vertexID / 3][vertexID % 3] << " " << wi[0] << " " << SHF[0] << " " << shCoef[0] << std::endl;
						}
					}

			//		//output
					for (int i = 0; i < coefNum; i++) shCoef[i] /= sampleNum, shCoef[i] *= 4 /** PI*/;
					outputFile << tempMesh.p_indices[vertexID / 3][vertexID % 3] << " ";
					for (int i = 0; i < coefNum; i++) outputFile << shCoef[i] << " ";
					outputFile << std::endl;

					vertexMap[tempMesh.p_indices[vertexID / 3][vertexID % 3]] = 1;

					if (DEBUG) {
						debugOutputFile << tempMesh.p_indices[vertexID / 3][vertexID % 3] << " ";
						glm::vec3 SH(0);
						for (int i = 9; i < 16; i++) SH += shCoef[i] * SHEnv[i];
						debugOutputFile << SH.x << " " << SH.y << " " << SH.z << " " << std::endl;
					}
				}

				std::cout << "\b\b\b\b";
				std::cout.width(3);
				std::cout << vertexID * 100 / (tempMesh.p_indices.size() * 3 - 1) << "%";
			}
			std::cout << std::endl;
			//outputFile.close();
			delete[] vertexMap;
		}
	}
	if (DEBUG) {
		debugOutputFile.close();
	}
}
void precomputeZHCOEF(std::shared_ptr<Scene> worldScene) {
	std::ofstream outputFile;

	std::string sceneName = worldScene->sceneName;
	int SHOrder = worldScene->getSHOrder();
	int coefNum = (1 + SHOrder) * (1 + SHOrder);
	int sampleNum = worldScene->getSampleNum();

	std::vector<glm::vec3> sampleOmega;

	std::ifstream inputFile(sceneName + "/" + "ENV_coef.txt");
	SHColor env_SH(SHOrder);
	SHColor env_ZHF(SHOrder);
	for (int i = 0; i < coefNum; i++) {
		for (int c = 0; c < 3; c++)
			inputFile >> env_SH[c][i] ;
	}
	std::vector<Eigen::MatrixXd> A;

	while (1) {
		sampleOmega.clear();
		for (int i = 0; i < SHOrder * 2 + 1; i++)
			sampleOmega.push_back(sphericalRandomSample());

		bool allInv = true;
		A.clear();
		int startIndex = 0;
		SHCoef temp(SHOrder);
		for (int l = 0; l <= SHOrder; l++) {
			int tempCoefNum = l * 2 + 1;
			Eigen::MatrixXd Y(tempCoefNum, tempCoefNum);
			for (int i = 0; i < tempCoefNum; i++) {
				SHFSample(temp, l, sampleOmega[i].x, sampleOmega[i].y, sampleOmega[i].z);
				for (int j = 0; j < tempCoefNum; j++)
					Y(i, j) = temp[startIndex + j];
			}
			if (Y.determinant() == 0) {
				allInv = false;
				continue;
			}
			A.push_back(Y.inverse().transpose());
			startIndex += tempCoefNum;
		}
		
		if (allInv) {
			int startIndex = 0;
			for (int l = 0; l <= SHOrder; l++) {
				int tempCoefNum = l * 2 + 1;
				Eigen::MatrixXd tempMat = A[l];
				
				if (DEBUG) {
					std::cout << "ORDER : " << l << std::endl;
					std::cout << A[l] << std::endl;
				}

				for (int i = 0; i < tempCoefNum; i++) 
					for (int j = 0; j < tempCoefNum; j++) {
						env_ZHF[0][startIndex + i] += env_SH[0][startIndex + j] * tempMat(i, j);
						env_ZHF[1][startIndex + i] += env_SH[1][startIndex + j] * tempMat(i, j);
						env_ZHF[2][startIndex + i] += env_SH[2][startIndex + j] * tempMat(i, j);
					}
				startIndex += tempCoefNum;
			}
			break;
		}
	}
	outputFile.open(sceneName + "/" + "ENV_ZHF_coef.txt");
	for (int i = 0; i < coefNum; i++) {
		for (int c = 0; c < 3; c++)
			outputFile << env_ZHF[c][i] << " ";
		outputFile << std::endl;
	}
	outputFile.close();
	outputFile.open(sceneName + "/" + "ENV_OMEGA_coef.txt");
	for (int i = 0; i < sampleOmega.size(); i++) {
		outputFile << sampleOmega[i].x << " " << sampleOmega[i].y << " " << sampleOmega[i].z << std::endl;
	}
	outputFile.close();
	outputFile.open(sceneName + "/" + "ENV_A_coef.txt");
	for (int i = 0; i <= SHOrder; i++) {
		outputFile << A[i] << std::endl;
	}
	outputFile.close();

	if (DEBUG) {
		std::ofstream outputY(sceneName + "/" + "ENV_Y.txt");
		int startIndex = 0;
		outputFile.open(sceneName + "/" + "ENV_ZHE_coef.txt");
		SHCoef temp(SHOrder);
		for (int l = 0; l <= SHOrder; l++) {
			int tempCoefNum = l * 2 + 1;
			Eigen::MatrixXd tempMat(tempCoefNum, tempCoefNum);
			for (int i = 0; i < tempCoefNum; i++) { 
				SHFSample(temp, l, sampleOmega[i].x, sampleOmega[i].y, sampleOmega[i].z);
				for (int j = 0; j < tempCoefNum; j++)
					tempMat(i, j) = temp[startIndex + j];
			}
			tempMat.transposeInPlace();
			for (int i = 0; i < tempCoefNum; i++) {
				for (int c = 0; c < 3; c++){
					double ZHE = 0;
					for (int j = 0; j < tempCoefNum; j++)
						ZHE += env_ZHF[c][startIndex + j] * tempMat(i,j);
					outputFile << ZHE << " ";
				}
				outputFile << std::endl;
			}
			startIndex += tempCoefNum;
			outputY << tempMat << std::endl;
		}
		outputFile.close();
		outputY.close();
	}

	std::cout << "Precompute ENV ZHF :  OK!\n";
}

void precomputeAreaLightCOEF(std::shared_ptr<Scene> worldScene) {

}



void precompute(std::shared_ptr<Scene> worldScene) {
	//environment light map sH precompute
	precomputeENV(worldScene);
	//PRT SH precompute
	precomputePRT(worldScene);
	//ZXZXZ ?

	//fast rotation ½üËÆ

	//zonal harmonics ZH precompute
	precomputeZHCOEF(worldScene);
	//area light
	precomputeAreaLightCOEF(worldScene);
}