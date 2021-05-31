#include <glad/glad.h>
#include <glfw/include/GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int displaySHORDER = 0;
#include "Shader.h"
#include "Camera.h"
#include "toolkit.h"
#include "Interaction.h"

const double PI = 4.0 * atan(1.0);
int DEBUG = 0;
const int kCacheSize = 16;

const int kHardCodedOrderLimit = 4;
//TEST
double DoubleFactorial(int x) {
	static const double dbl_factorial_cache[kCacheSize] = { 1, 1, 2, 3, 8, 15, 48, 105,
													384, 945, 3840, 10395, 46080,
													135135, 645120, 2027025 };

	if (x < kCacheSize) {
		return dbl_factorial_cache[x];
	}
	else {
		double s = dbl_factorial_cache[kCacheSize - (x % 2 == 0 ? 2 : 1)];
		double n = x;
		while (n >= kCacheSize) {
			s *= n;
			n -= 2.0;
		}
		return s;
	}
}
double EvalLegendrePolynomial(int l, int m, double x) {
	// Compute Pmm(x) = (-1)^m(2m - 1)!!(1 - x^2)^(m/2), where !! is the double
	// factorial.
	double pmm = 1.0;
	// P00 is defined as 1.0, do don't evaluate Pmm unless we know m > 0
	if (m > 0) {
		double sign = (m % 2 == 0 ? 1 : -1);
		pmm = sign * DoubleFactorial(2 * m - 1) * pow(1 - x * x, m / 2.0);
	}

	if (l == m) {
		// Pml is the same as Pmm so there's no lifting to higher bands needed
		return pmm;
	}

	// Compute Pmm+1(x) = x(2m + 1)Pmm(x)
	double pmm1 = x * (2 * m + 1) * pmm;
	if (l == m + 1) {
		// Pml is the same as Pmm+1 so we are done as well
		return pmm1;
	}

	// Use the last two computed bands to lift up to the next band until l is
	// reached, using the recurrence relationship:
	// Pml(x) = (x(2l - 1)Pml-1 - (l + m - 1)Pml-2) / (l - m)
	for (int n = m + 2; n <= l; n++) {
		double pmn = (x * (2 * n - 1) * pmm1 - (n + m - 1) * pmm) / (n - m);
		pmm = pmm1;
		pmm1 = pmn;
	}
	// Pmm1 at the end of the above loop is equal to Pml
	return pmm1;
}
double Factorial(int x) {
	static const double factorial_cache[kCacheSize] = { 1, 1, 2, 6, 24, 120, 720, 5040,
												40320, 362880, 3628800, 39916800,
												479001600, 6227020800,
												87178291200, 1307674368000 };

	if (x < kCacheSize) {
		return factorial_cache[x];
	}
	else {
		double s = factorial_cache[kCacheSize - 1];
		for (int n = kCacheSize; n <= x; n++) {
			s *= n;
		}
		return s;
	}
}
#define M_PI 3.141592653589793f
double EvalSHSlow(int l, int m, double phi, double theta) {
	double kml = sqrt((2.0 * l + 1) * Factorial(l - abs(m)) /
		(4.0 * M_PI * Factorial(l + abs(m))));
	if (m > 0) {
		return sqrt(2.0) * kml * cos(m * phi) *
			EvalLegendrePolynomial(l, m, cos(theta));
	}
	else if (m < 0) {
		return sqrt(2.0) * kml * sin(-m * phi) *
			EvalLegendrePolynomial(l, -m, cos(theta));
	}
	else {
		return kml * EvalLegendrePolynomial(l, 0, cos(theta));
	}
}

#include "Scene.h"
#include "hdrMapGen.hpp"
#include "precompute.hpp"
#include "realTimeRender.hpp"
#include "hardCodeCoefGenTool.hpp"

int gVarReprecompute = false;
std::shared_ptr<Scene> worldScene;

int main(int argc, char** argv) {
	std::ofstream oFile("debug/temp.txt");
	SHCoef temp(15);
	SHFSample(temp, 15, 1, 0, 0);
	for (int l = 0; l <= 15; l++) 
		for (int m = -l; m <= l; m++) {
			double a = temp[l * l + m + l];
			double b = EvalSHSlow(l, m, 0, M_PI / 2);
			oFile << a << std::setw(20) << b << std::setw(20) << fabs(a - b) << std::endl;
		}
	oFile.close();

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