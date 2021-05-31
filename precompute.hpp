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

void filter(int l, Float a, SHCoef& sh){
	int offset = l * l;
	for (int m = -l; m <= l; ++m, ++offset)	
		sh[offset] *= a;
}
void hanning(int w, int SH_ORDER, SHCoef& sh) {
	for (int l = 0; l < SH_ORDER; ++l)
		if (l > w)
			filter(l, 0, sh);
		else
			filter(l, (std::cos(PI * Float(l) / Float(w)) + 1.0f) * 0.5f, sh);
}
int hanningLevel = 20;

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
			//hanning(hanningLevel, SHOrder, SHF);

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
					for (int i = 0; i < coefNum; i++) shCoef[i] = 0;
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

							SHFSample(SHF, SHOrder, wi.x, wi.y, wi.z);
							//hanning(hanningLevel, SHOrder, SHF);

							for (int i = 0; i < coefNum; i++) shCoef[i] += SHF[i] * func_value;
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
		//sampleOmega.push_back(glm::vec3(1, 0, 0));
		//sampleOmega.push_back(glm::vec3(0, 1, 0));
		//sampleOmega.push_back(glm::vec3(0, 0, 1));
		//sampleOmega.push_back(glm::vec3(0.373805, 0.927153, 0.0256307));
		//sampleOmega.push_back(glm::vec3(-0.431649, 0.632692, 0.642946));
		//sampleOmega.push_back(glm::vec3(0.801329, -0.430057, 0.415840));
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
				//hanning(6, SHOrder, temp);
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
		outputFile << A[i].transpose() << std::endl;
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
				//hanning(6, SHOrder, temp);
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

float Pmem[100];
bool Pflag[100];
float P(int l, float x, float* pmem, bool* pflag) {
	if (l == 0)
		return 1;
	else if (l == 1)
		return x;
	if (pflag[l]) return pmem[l];
	//std::cout << l << std::endl;
	float p1 = P(l - 1, x, pmem, pflag);
	float p2 = P(l - 2, x, pmem, pflag);

	pflag[l] = 1;
	pmem[l] = x * (2 * l - 1) / l * p1 - 1.0 * (l - 1) / l * p2;
	return pmem[l];
}
float C_CALC(int l, float a, float b, float gamma, float D, float B) {
	for (int i = 0; i < l; i++) Pflag[i] = 0;
	float P1 = P(l - 1, a * cos(gamma) + b * sin(gamma), Pmem, Pflag);
	for (int i = 0; i < l; i++) Pflag[i] = 0;
	float P2 = P(l - 1, a, Pmem, Pflag);

	return
		((a * sin(gamma) - b * cos(gamma)) * P1 +
			b * P2 +
			(a * a + b * b - 1) * D +
			(l - 1) * B) / l;
}
void precomputeAreaLightCOEFCPU(std::shared_ptr<Scene> worldScene) {
	int SHOrder = worldScene->getSHOrder();
	int coefNum = worldScene->getSHCoefNum();

	glm::vec3* W;
	GLfloat* A;
	//uW
	W = new glm::vec3[(SHOrder * 2 + 1) * 3];
	std::ifstream input(worldScene->sceneName + "/" + "ENV_OMEGA_coef.txt");
	for (int i = 0; i < SHOrder * 2 + 1; i++) {
		glm::vec3 temp;
		input >> W[i].x >> W[i].y >> W[i].z;
	}
	input.close();
	//uA
	A = new GLfloat[(SHOrder + 1) * (SHOrder * 2 + 3) * (SHOrder * 2 + 1) / 3];//(k+1)(2k+3)(2k+1)/3
	input.open(worldScene->sceneName + "/" + "ENV_A_coef.txt");
	int index = 0;
	for (int l = 0; l <= SHOrder; l++)
		for (int i = 0; i < SHOrder * 2 + 1; i++) {
			for (int j = 0; j < SHOrder * 2 + 1; j++)
				input >> A[index++];
		}
	input.close();
	//uL
	for (int modelID = 0; modelID < worldScene->world.size(); modelID++) {
		model tempModel = worldScene->world[modelID];
		for (int meshID = 0; meshID < tempModel.meshes.size(); meshID++) {
			mesh tempMesh = tempModel.meshes[meshID];
			std::cout << "Precompute PRT AREALIGHT : " << tempModel.objName << " - MESH : " << meshID << " :   0%";

			std::ofstream outputFile(worldScene->sceneName + "/" + "PRT_" + tempModel.objName + "_AREALIGHT_" + std::to_string(meshID) + "_coef.txt");
			for (int vertexID = 0; vertexID < tempMesh.p.size(); vertexID++) {
				GLfloat* Lout = new GLfloat[(SHOrder + 1) * (SHOrder + 1) * 3];
				for (int i = 0; i < (SHOrder + 1) * (SHOrder + 1) * 3; i++) Lout[i] = 0;

				for (int lightID = 0; lightID < worldScene->light.size(); lightID++) {
					areaLight tempLight = worldScene->light[lightID];
					int lightPointsSize = worldScene->light[lightID].p.size();
					//uV
					glm::vec3* V = new glm::vec3[lightPointsSize];
					for (int i = 0; i < lightPointsSize; i++)
						V[i].x = tempLight.p[i].x,
						V[i].y = tempLight.p[i].y,
						V[i].z = tempLight.p[i].z;
					//project to sphere
					glm::vec3* V_ = new glm::vec3[lightPointsSize];
					glm::vec3* W_ = new glm::vec3[lightPointsSize];
					for (int e = 0; e < lightPointsSize; e++) {
						V_[e] = V[e] - tempMesh.p[vertexID];
						W_[e] = glm::normalize(V_[e]); W_[e] = glm::vec3(W_[e].z, W_[e].y, W_[e].x);
					}
					delete[] V;
					//pre calc per edge
					glm::vec3* lambda = new glm::vec3[lightPointsSize];
					glm::vec3* mu = new glm::vec3[lightPointsSize];
					GLfloat* gamma = new GLfloat[lightPointsSize];
					//A DIR CHECK : NOT METION IN PAPER
					float dir = 1;
					if (dot(glm::cross(W_[1] - W_[0], W_[2] - W_[1]), W_[1]) < 0) dir = -1;

					for (int e = 0; e < lightPointsSize; e++) {
						glm::vec3 omega1 = W_[e];
						glm::vec3 omega2 = W_[(e + 1) % lightPointsSize];

						mu[e] = glm::normalize(glm::cross(omega1, omega2));
						lambda[e] = glm::normalize(glm::cross(mu[e], omega1));
						gamma[e] = glm::acos(glm::dot(omega1, omega2));
					}
					//soild Angle
					GLfloat* S_ = new GLfloat[SHOrder + 1];
					S_[0] = 0;
					for (int e = 0; e < lightPointsSize; e++) {
						glm::vec3 omega1 = W_[((e - 1) + lightPointsSize) % lightPointsSize];
						glm::vec3 omega2 = W_[e];
						glm::vec3 omega3 = W_[(e + 1) % lightPointsSize];
						S_[0] += glm::acos(
							dot(
								glm::normalize(glm::cross(omega2, omega1)),
								glm::normalize(glm::cross(omega2, omega3)))
						);
						//std::cout << glm::dot(
						//	glm::normalize(glm::cross(omega2, omega1)),
						//	glm::normalize(glm::cross(omega2, omega3))) << std::endl;
					}
					S_[0] -= (lightPointsSize - 2) * PI;
					S_[0] *= dir;
					GLfloat* L = new GLfloat[(SHOrder + 1) * (SHOrder * 2 + 1)];
					for (int i = 0; i < SHOrder * 2 + 1; i++) {
						glm::vec3 omega = glm::vec3(W[i].z, W[i].y, W[i].x);
						S_[1] = 0;
						GLfloat* a = new GLfloat[lightPointsSize];
						GLfloat* b = new GLfloat[lightPointsSize];
						GLfloat* c = new GLfloat[lightPointsSize];
						GLfloat* B = new GLfloat[lightPointsSize * (SHOrder + 1)];
						GLfloat* D = new GLfloat[lightPointsSize * (SHOrder + 1)];
						GLfloat* B_ = new GLfloat[SHOrder + 1]; B_[0] = B_[1] = 0;
						for (int e = 0; e < lightPointsSize; e++) {
							a[e] = glm::dot(omega, W_[e]);
							b[e] = glm::dot(omega, lambda[e]);
							c[e] = glm::dot(omega, mu[e]);
							S_[1] = S_[1] + c[e] * gamma[e] / 2;
							B[0 * lightPointsSize + e] = gamma[e];
							B[1 * lightPointsSize + e] = a[e] * sin(gamma[e]) - b[e] * cos(gamma[e]) + b[e];
							D[0 * lightPointsSize + e] = 0;
							D[1 * lightPointsSize + e] = gamma[e];
							B_[0] += B[0 * lightPointsSize + e] * c[e];
							B_[1] += B[1 * lightPointsSize + e] * c[e];
						}

						GLfloat* C = new GLfloat[lightPointsSize * (SHOrder + 1)];
						for (int l = 2; l <= SHOrder; l++) {
							B_[l] = 0;
							for (int e = 0; e < lightPointsSize; e++) {
								C[(l - 1) * lightPointsSize + e] = C_CALC(l, a[e], b[e], gamma[e], D[(l - 1) * lightPointsSize + e], B[(l - 2) * lightPointsSize + e]);
								B[l * lightPointsSize + e] = C[(l - 1) * lightPointsSize + e] * (2 * l - 1) / l - B[(l - 2) * lightPointsSize + e] * (l - 1) / l;
								B_[l] += c[e] * B[l * lightPointsSize + e];
								D[l * lightPointsSize + e] = (2 * l - 1) * B[(l - 1) * lightPointsSize + e] + D[(l - 2) * lightPointsSize + e];
							}
							S_[l] = B_[l - 1] * (2 * l - 1) / (l * (l + 1)) + S_[l - 2] * (l - 2) * (l - 1) / (l * (l + 1));
						}

						for (int l = 0; l <= SHOrder; l++) {
							L[l * (SHOrder * 2 + 1) + i] = sqrt(1.0 * (2 * l + 1) / 4 / PI) * S_[l] * dir;
						}

						delete[] a;
						delete[] b;
						delete[] c;
						delete[] B;
						delete[] D;
						delete[] B_;
						delete[] C;
					}
					int index = 0;
					for (int l = 0; l <= SHOrder; l++) {
						for (int m = -l; m <= l; m++)
							for (int d = 0; d < 2 * l + 1; d++) {
								Lout[(l * l + m + l) * 3 + 0] += A[index + (m + l) * (2 * l + 1) + d] * L[l * (SHOrder * 2 + 1) + d] * tempLight.color.r;
								Lout[(l * l + m + l) * 3 + 1] += A[index + (m + l) * (2 * l + 1) + d] * L[l * (SHOrder * 2 + 1) + d] * tempLight.color.g;
								Lout[(l * l + m + l) * 3 + 2] += A[index + (m + l) * (2 * l + 1) + d] * L[l * (SHOrder * 2 + 1) + d] * tempLight.color.b;
							}
						index += (2 * l + 1) * (2 * l + 1);
					}

					delete[] V_;
					delete[] W_;
					delete[] lambda;
					delete[] mu;
					delete[] gamma;
					delete[] S_;
					delete[] L;
				}
				//Sum	
				int index = 0;
				for (int l = 0; l <= SHOrder; l++)
					for (int m = -l; m <= l; m++)
						outputFile << Lout[(l * l + m + l) * 3 + 0] << " " << Lout[(l * l + m + l) * 3 + 1] << " " << Lout[(l * l + m + l) * 3 + 2] << " ";
				
				std::cout << "\b\b\b\b";
				std::cout.width(3);
				std::cout << vertexID * 100 / (tempMesh.p.size() - 1) << "%";
				//output
				outputFile << std::endl;

				delete[] Lout;
			}
			outputFile.close();
		} 
	}
	std::cout << std::endl;
	delete[] W;
	delete[] A;
	return;
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
	//done in real Time for using OpenGL Shader
	precomputeAreaLightCOEFCPU(worldScene);
}