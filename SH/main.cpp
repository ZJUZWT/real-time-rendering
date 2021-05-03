#include <glad/glad.h>
#include <glfw/include/GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "toolkit.h"
#include "Interaction.h"

const double PI = 4.0 * atan(1.0);
int DEBUG = 0;

#include "Scene.h"
#include "hdrMapGen.hpp"
#include "precompute.hpp"
#include "realTimeRender.hpp"
#include "hardCodeCoefGenTool.hpp"

int gVarReprecompute = false;
std::shared_ptr<Scene> worldScene;

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "ERROR : Input A <xxx.config> file!" << std::endl;
		getchar(); return -1;
	}
	for (int i = 1, countCONFIG = 0; i < argc; i++) {
		std::string token(argv[i]);
		if (token == "-h" || token == "-H" || token == "-HardCodeCoefGenterate") {
			hardCodeCoefGen();
			return 0;
		}
		if (token == "-m" || token == "-M" || token == "-MapHDRGenterate") {
			if (i == argc - 1) {
				std::cout << "ERROR : Input -m <Your path> !" << std::endl;
			}
			else {
				std::string outputPath(argv[i + 1]);
				hdrMapGen(outputPath);
			}
			return 0;
		}
		if (token == "-r" || token == "-R" || token == "-Reprecompute") {
			gVarReprecompute = true;
			continue;
		}
		if (token == "-d" || token == "-D" || token == "-Debug") {
			DEBUG = 1;
			continue;
		}
		if (token.find(".config")) {
			if (countCONFIG == 0) {
				countCONFIG += 1;
				worldScene = Scene::loadScene(token);
			}
			else {
				std::cout << "WANRING : Only read the first <xxx.config>!" << std::endl;
			}
			continue;
		}
	}
	printf("CEHCK CONFIG READ\n");

	if (gVarReprecompute) {
		bool run = false;
		std::ifstream envLightCoef(worldScene->sceneName + "_EL_coef.txt");
		if (!envLightCoef.is_open()) run = true; else envLightCoef.close();

		if (!run)
			for (int i = 0; i < worldScene->world.size(); i++) {
				std::ifstream transCoef(worldScene->sceneName + "_PRT_" + worldScene->world[i].objName + "_coef.txt");
				if (!transCoef.is_open()) {
					run = true; break;
				}
				transCoef.close();
			}
		if (run) precompute(worldScene);
	}
	printf("CEHCK PRECOMPUTE\n");
	realTimeRender(worldScene);
}