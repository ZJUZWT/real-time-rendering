#pragma once
//#define DEBUG_ROTATE

#include <ctime>
#include <random>
#include "SHFSampler.h"
#include "SHRotateMat.h"

std::default_random_engine random;

class Image {
public:
	Image(int w, int h, int d) : width(w), height(h), depth(d) { data = new Float[w * h * d]; }
	Image(int w, int h, int d, float* dat) : width(w), height(h), depth(d) { data = new Float[w * h * d]; for (int i = 0; i < w * h * d; i++) data[i] = dat[i]; }
	Float& at(int i, int j, int k) { return data[(i + j * width) * depth + k]; }
	Float at(int i, int j, int k) const { return data[(i + j * width) * depth + k]; }
	Float at(int i) const { return data[i]; }
	int width, height, depth;
private:
	Float* data;
};
float sinc(float x) {               /* Supporting sinc function */
	if (fabs(x) < 1.0e-4) return 1.0;
	else return(sin(x) / x);
}
bool sphereMapping(int u, int v, const int& w, const int& h, Float& x, Float& y, Float& z, Float& domega) {
	//将平面uv映射到球面
	//float theta = v * 1.0 * PI / (w - 1);
	//float phi = u * 2.0 * PI / (h - 1);

	//x = sin(theta) * cos(phi);
	//y = sin(theta) * sin(phi);
	//z = cos(theta); return true;

	Float uu, vv, r, theta, phi;

	vv = (w / 2.0 - u) / (w / 2.0);		// vv ranges from -1 to 1
	uu = (v - h / 2.0) / (h / 2.0);		// uu ranges from -1 to 1
	r = sqrt(uu * uu + vv * vv);

	// Consider only circle with r < 1
	if (r > 1.0)
		return false;

	theta = PI * r;
	phi = atan2(vv, uu);

	// Cartesian components
	x = sin(theta) * cos(phi);
	y = sin(theta) * sin(phi);
	z = cos(theta);

	// Computation of the solid angle.  This follows from some elementary calculus converting sin(theta) d theta d phi into
	// coordinates in terms of r.  This calculation should be redone if the form of the input changes
	domega = (2 * PI / w) * (2 * PI / h) * sinc(theta);

	return true;
}
void filter(int l, Float a, SHCoef& sh)
{
	int offset = l * l;
	for (int m = -l; m <= l; ++m, ++offset)
	{
		sh[offset] *= a;
	}
}
void gauss(int w, SHCoef& sh)
{
	for (int l = 0; l < SH_ORDER; ++l)
	{
		filter(l, std::exp(-std::pow(PI * Float(l) / Float(w), 2.0f) / 2.0f), sh);
	}
}
void hanning(int w, SHCoef& sh)
{
	for (int l = 0; l < SH_ORDER; ++l)
	{
		if (l > w)
			filter(l, 0, sh);
		else
			filter(l, (std::cos(PI * Float(l) / Float(w)) + 1.0f) * 0.5f, sh);
	}
}

void lanczos(int w, SHCoef& sh)
{
	for (int l = 0; l < SH_ORDER; ++l)
	{
		if (l == 0)
			filter(l, 1, sh);
		else
			filter(l, std::sin(PI * Float(l) / Float(w)) / (PI * Float(l) / Float(w)), sh);
	}
}
SHColor paper2_rotateY(Float angle, SHColor iCoef, const Float* dySubDiag, const Float* ddyDiag) {
	SHColor oCoef;
	float beta2 = 0.5f * angle * angle;
	for (int c = 0; c < 3; c++) {
		oCoef[c][0] = iCoef[c][0];
		int i;
		for (i = 1; i < SH_ORDER * SH_ORDER - 1; i++) {
			Float ddyCoef = beta2 * ddyDiag[i];
			Float dyCoef1 = angle * dySubDiag[i];
			Float dyCoef2 = angle * dySubDiag[i + 1];

			//oCoef[c][i] = iCoef[c][i] * (1.0 + iCoef[c][i] * beta2 * ddyDiag[i]) +	angle * (dySubDiag[i] * iCoef[c][i - 1] - dySubDiag[i + 1] * iCoef[c][i + 1]);
			oCoef[c][i] = iCoef[c][i] * 1.0 + iCoef[c][i] * ddyCoef + dyCoef1 * iCoef[c][i - 1] - dyCoef2 * iCoef[c][i + 1];
		}
		oCoef[c][i] = iCoef[c][i] * (1.0 + beta2 * ddyDiag[i]) + angle * dySubDiag[i] * iCoef[c][i - 1];
	}
	return oCoef;
}
//图上面就是逆时针旋转
SHColor paper2_rotateZ(Float angle, SHColor iCoef) {
	SHColor oCoef;
	for (int i = 0; i < 3; i++)
		for (int l = 0; l < SH_ORDER; l++) {
			int center = l * l + l;
			oCoef[i][center] = iCoef[i][center];
			for (int m = 1; m <= l; m++) {
				oCoef[i][center - m] = iCoef[i][center - m] * cos(m * angle) - iCoef[i][center + m] * sin(m * angle);
				oCoef[i][center + m] = iCoef[i][center - m] * sin(m * angle) + iCoef[i][center + m] * cos(m * angle);
			}
		}
	return oCoef;
}

const int MCNum = 2000000;
class SH {
public:
	SH() {};
	//投影
	void projection(Image& img, SHCoef* sampleMap) {
		//for (int i = 0; i < MCNum; i++){
		for (int w = 0; w < img.width; w++)
			for (int h = 0; h < img.height; h++) {
				//We now find the cartesian components for the point (i,j)
				//int w = random() % img.width;
				//int h = random() % img.height;
				Float x, y, z, domega = 4 * PI / img.width / img.height;
				if (sphereMapping(w, h, img.width, img.height, x, y, z, domega)) {
					//这相当于我们的原函数采样		
					Float color[3] = { img.at(w, h, 0), img.at(w, h, 1), img.at(w,h,2) };
					//Sample the SH function，这里是实采样	
					sampleMap[h * img.width + w] = SHFSample(x, y, z);
					gauss(SH_ORDER, sampleMap[h * img.width + w]);
					//采样之后用蒙特卡洛积分的方法，domega是简化的概率
					for (int c = 0; c < 3; ++c)
						for (int k = 0; k < SH_COEFNUM; ++k)
							originCoef[c][k] += sampleMap[h * img.width + w][k] * color[c] * domega;
				}
			}
	};
	//重建
	void reconstruction(Image& img, SHCoef* sampleMap, int mode = 0) {
		for (int i = 0; i < img.width; i++)
			for (int j = 0; j < img.height; j++) {
				// We now find the cartesian components for the point (i,j)
				Float x, y, z, domega = 4 * PI / img.width / img.height;
				if (sphereMapping(i, j, img.width, img.height, x, y, z, domega)) {
					//Sample the SH function，这里是实采样
					//SHCoef sampledSHF = SHFSample(x, y, z);
					//结合我们求出来的系数数组重建image
					for (int c = 0; c < 3; ++c) {
						img.at(i, j, c) = 0;
						//if (c < 2 || c > 2) continue;
						for (int k = 0; k < SH_COEFNUM; ++k)
							if (mode == 0) img.at(i, j, c) += sampleMap[j * img.width + i][k] * originCoef[c][k];
							else img.at(i, j, c) += sampleMap[j * img.width + i][k] * rotateCoef[c][k];
					}
				}
			}
	}
	void paper2_ZYZrotate(Float alpha = 0, Float beta = 0, Float gamma = 0) {
		rotateCoef = paper2_rotateZ(alpha, originCoef);
#ifdef DEBUG_ROTATE
		std::cout << std::setprecision(5);
		for (int i = 0; i < SH_COEFNUM; i++) std::cout << rotateCoef[0][i] << "\t"; std::cout << std::endl;
#endif // DEBUG_ROTATE
		rotateCoef = paper2_rotateY(beta, rotateCoef, rotateMat.getDySubDiag(), rotateMat.getDdyDiag());
#ifdef DEBUG_ROTATE
		std::cout << std::setprecision(5);
		for (int i = 0; i < SH_COEFNUM; i++) std::cout << rotateCoef[0][i] << "\t"; std::cout << std::endl;
#endif // DEBUG_ROTATE
		rotateCoef = paper2_rotateZ(gamma, rotateCoef);
	}
	~SH() {}

	SHColor& getOriginCoef() { return originCoef; }
	SHRotateMat& getRotateMat() { return rotateMat; }
private:
	SHColor originCoef;
	SHColor rotateCoef;
	SHRotateMat rotateMat;
};