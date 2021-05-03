#pragma once
#include "mesh.h"
#include "SHFSampler.h"
#include <random>

glm::vec3 sphericalRandomSample() {
	std::default_random_engine e;
	std::uniform_real_distribution<Float> rand(-1.0, 1.0);

	Float u = rand(e);
	Float v = rand(e);
	Float r2 = u * u + v * v;

	while (r2 > 1.0) {
		u = rand(e);
		v = rand(e);
		r2 = u * u + v * v;
	}
	return glm::vec3(2 * u * sqrt(1 - r2), 2 * v * sqrt(1 - r2), 1 - 2 * r2);
}

void precomputeENV(std::shared_ptr<Scene> worldScene) {
	std::ofstream outputFile;

	std::string sceneName = worldScene->sceneName;
	int SHNum = worldScene->getSHNum();
	int coefNum = (1 + SHNum) * (1 + SHNum);

	int width, height, nrChannels;
	std::string path = worldScene->envMap;
	float* data = stbi_loadf(path.c_str(), &width, &height, &nrChannels, 0);

	SHColor env_SH(SHNum);
	for (int i = 0; i < coefNum; i++)
		for (int c = 0; c < nrChannels; c++)
			env_SH[c][i] = 0;

	std::cout << "Precompute ENV :   0%";

	for (int v = 0; v < height; v++) { //ÀèÂüºÍ
		Float area = 2 * PI * (cos(v * 1.0 / (height + 1)) - cos((v * 1.0 + 1) / (height + 1))) / width;
		for (int u = 0; u < width; u++) {
			Float Li[3] = {
				data[(v * width + u) * nrChannels + 0],
				data[(v * width + u) * nrChannels + 1],
				data[(v * width + u) * nrChannels + 2]};

			Float
				//y = cos(1.0 * v / height * PI),//stb vertical reverse
				//x = sqrt(1 - y * y) * cos((u * 1.0 / width - 0.5) * PI * 2),
				//z = sqrt(1 - y * y) * sin((u * 1.0 / width - 0.5) * PI * 2);
				y = cos((1.0 * v / height * PI)),
				x = sqrt(1 - y * y) * cos((u * 1.0 / width) * PI * 2),
				z = sqrt(1 - y * y) * sin((u * 1.0 / width) * PI * 2);

			auto SHF = SHFSample(SHNum + 1, x, y, z);
 
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

	outputFile.open(sceneName + "_ENV_coef.txt");
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
	int SHNum = worldScene->getSHNum();
	int coefNum = (1 + SHNum) * (1 + SHNum);
	int sampleNum = worldScene->getSampleNum();

	for (int meshID = 0; meshID < worldScene->world.size(); meshID++) {
		mesh &tempMesh = worldScene->world[meshID];
		std::cout << "Precompute PRT MESH " << tempMesh.objName << " :   0%";
		unsigned int* vertexMap = new unsigned int[tempMesh.p.size()]{0};

		outputFile.open(sceneName + "_PRT_" + tempMesh.objName + "_coef.txt");

		for (int vertexID = 0; vertexID < tempMesh.p_indices.size() * 3; vertexID++) {
			//std::cout << tempMesh.p_indices.size() << std::endl;
			//std::cout << PI << std::endl;

			if (!vertexMap[tempMesh.p_indices[vertexID / 3][vertexID % 3]]) {
				glm::vec3 p = tempMesh.p[tempMesh.p_indices[vertexID / 3][vertexID % 3]];
				glm::vec3 n = tempMesh.n[tempMesh.n_indices[vertexID / 3][vertexID % 3]];

				SHCoef shCoef(SHNum);

				//sample
				//for (int i = 0; i < sampleNum; i++) {
				//	glm::vec3 wi = sphericalRandomSample();	wi /= wi.length();

				//	Float NdotW = glm::dot(n, wi);

				//	if (NdotW < 0) continue;
				//	//MC sample 
				//	//pdf = 1/4/pi
				//	auto SHF = SHFSample(SHNum + 1, wi[0], wi[1], wi[2]);

				//	for (int i = 0; i < coefNum; i++) shCoef[i] += SHF[i] * NdotW;
				//}
				//sample2 reference
				int sample_side = sqrt(sampleNum);
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_real_distribution<> rng(0.0, 1.0);
				for (int t = 0; t < sample_side; t++) {
					for (int p = 0; p < sample_side; p++) {
						double alpha = (t + rng(gen)) / sample_side;
						double beta = (p + rng(gen)) / sample_side;

						double phi = 2.0 * PI * beta;
						double theta = acos(2.0 * alpha - 1.0);

						glm::vec3 wi(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
						Float NdotW = glm::dot(n, wi);
						if (NdotW < 0) continue;
						double func_value = NdotW;

						auto SHF = SHFSample(SHNum + 1, wi[0], wi[1], wi[2]);

						for (int i = 0; i < coefNum; i++) shCoef[i] += SHF[i] * NdotW;
					}
				}

				//output
				for (int i = 0; i < coefNum; i++) shCoef[i] /= sampleNum, shCoef[i] *= 4 * PI;
				outputFile << tempMesh.p_indices[vertexID / 3][vertexID % 3] << " ";
				for (int i = 0; i < coefNum; i++) outputFile << shCoef[i] << " ";
				outputFile << std::endl;

				vertexMap[tempMesh.p_indices[vertexID / 3][vertexID % 3]] = 1;
			}

			std::cout << "\b\b\b\b";
			std::cout.width(3);
			std::cout << vertexID * 100 / (tempMesh.p_indices.size() * 3 - 1) << "%";
		}
		std::cout << std::endl;
		outputFile.close();
		delete[] vertexMap;
	}
}

void precompute(std::shared_ptr<Scene> worldScene) {
	//environment light map sH precompute
	precomputeENV(worldScene);
	//PRT SH precompute
	precomputePRT(worldScene);
	//fast rotation ½üËÆ

	//zonal harmonics ZH precompute

	//area light

}