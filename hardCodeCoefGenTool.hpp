#define _CRT_SECURE_NO_WARNINGS
#pragma once

#include <cmath>
#include <cstdio>
#include <iostream>

//Refer to the paper : 2013 Efficient Spherical Harmonic Evaluation
const int level = 20;
const int listSize = level * level;
//const double PI = 4.0 * atan(1.0);

double KClac(const unsigned int l, const int m) {
	const unsigned int cAM = abs(m);
	double uVal = 1;// must be double
	for (unsigned int k = l + cAM; k > (l - cAM); k--) uVal *= k;
	return sqrt((2.0 * l + 1.0) / (4 * PI * uVal));
}

double K[listSize];
double Pmm[level];

void genLdist(int l, int dist);

void hardCodeCoefGen() {
	freopen("SHFSampler.h", "w", stdout);
	printf("#include \"SHStrutct.h\"\n");

	//Clac K
	for (int l = 0; l < level; l++)
		for (int m = 0; m <= l * 2; m++)
			K[l * l + m] = KClac(l, m - l);
	//Clac P
	Pmm[0] = 1;
	for (int l = 1; l < level; l++) Pmm[l] = (1 - 2 * l) * Pmm[l - 1];


	//output 
	for (int l = 1; l < level; l++) {
		std::cout << "void SHFSampler_" << l - 1 << "(SHCoef& pSH, const Float fX, const Float fY, const Float fZ) {" << std::endl;
		std::cout << "\t" << "Float fC0, fC1, fS0, fS1, fTmpA, fTmpB, fTmpC; Float fZ2 = fZ * fZ; " << std::endl;

		genLdist(l, 0);
		for (int dist = 1; dist < l; dist++) {//这个意味着从中心到两边的距离，递推计算
			if (dist == 1) printf("\n\tfC0 = fX;\n\tfS0 = fY;\n");
			else printf("\n\tfC%d = fX*fC%d - fY*fS%d;\n\tfS%d = fX*fS%d + fY*fC%d;\n", 1 - dist % 2, dist % 2, dist % 2, 1 - dist % 2, dist % 2, dist % 2);
			genLdist(l, dist);
		}

		std::cout << "}" << std::endl;
	}
	printf("void SHFSample(SHCoef& res, const int SH_ORDER, Float x, Float z, Float y) {\n");
	printf("\tFloat r = sqrt(x * x + y * y + z * z);\n\tx /= r;\n\ty /= r;\n\tz /= r;\n");

	for (int l = 1; l < level - 1; l++)
		printf("\tif (SH_ORDER == %d) SHFSampler_%d(res,x,y,z);\n\t\telse ", l-1, l-1);
	printf("SHFSampler_%d(res,x,y,z);\n}", level - 2);
}

void genLdist(int l, int dist) {
	//基本的代码生成方式就是前四位用4a 4b 4d 4e然后用4c
	if (dist == 0) {
		for (int i = 0; i < l; i++)
			if (i == 0) printf("\tpSH[0] = %.20lf;\n", K[0]);
			else if (i == 1) printf("\tpSH[2] = %.20lf*fZ;\n", K[2]);
			else if (i == 2) printf("\tpSH[6] = %.20lf*fZ2 - %.20lf;\n", K[6] * 3 / 2, K[6] / 2);
			else if (i == 3) printf("\tpSH[12] = fZ*(%.20lf*fZ2 - %.20lf);\n", K[12] * 5 * 3 / 6, K[12] * 9 / 6);
			else {
				int index1 = i * i + i;
				int index2 = (i - 1) * (i - 1) + i - 1;
				int index3 = (i - 2) * (i - 2) + i - 2;
				printf("\tpSH[%d] = %.20lf*fZ*pSH[%d] - %.20lf*pSH[%d];\n",
					index1,
					K[index1] / K[index2] * (2 * i - 1) / i,
					index2,
					K[index1] / K[index3] * (i - 1) / i,
					index3);
			}
	}
	else {
		double sqrt2 = sqrt(2);
		for (int i = 0; i < l - dist; i++) {
			int tempLevel = i + dist;
			int index1 = tempLevel * tempLevel + tempLevel;
			if (i == 0) printf("\tfTmp%c = %.20lf;\n", i % 3 + 'A',
				sqrt2 * K[index1 + dist] * Pmm[dist]);
			else if (i == 1) printf("\tfTmp%c = %.20lf*fZ;\n", i % 3 + 'A',
				sqrt2 * (2.0 * dist + 1.0) * K[index1 + dist] * Pmm[dist]);
			else if (i == 2) printf("\tfTmp%c = %.20lf*fZ2 - %.20lf;\n", i % 3 + 'A',
				sqrt2 * K[index1 + dist] * (2.0 * dist + 3) * (2.0 * dist + 1) / 2 * Pmm[dist],
				sqrt2 * K[index1 + dist] * (2.0 * dist + 1) / 2 * Pmm[dist]);
			else if (i == 3) printf("\tfTmp%c = fZ*(%.20lf*fZ2 - %.20lf);\n", i % 3 + 'A',
				sqrt2 * K[index1 + dist] * (2.0 * dist + 5.0) * (2.0 * dist + 3.0) * (2.0 * dist + 1.0) / 6 * Pmm[dist],
				sqrt2 * K[index1 + dist] * (4.0 * dist * dist + 8.0 * dist + 3.0) / 2.0 * Pmm[dist]);
			else {
				int index2 = (tempLevel - 1) * (tempLevel - 1) + tempLevel - 1;
				int index3 = (tempLevel - 2) * (tempLevel - 2) + tempLevel - 2;
				printf("\tfTmp%c = %.20lf*fZ*fTmp%c - %.20lf*fTmp%c;\n", i % 3 + 'A',
					K[index1 + dist] / K[index2 + dist] * (2.0 * tempLevel - 1.0) / i,
					(i + 2) % 3 + 'A',
					K[index1 + dist] / K[index3 + dist] * (tempLevel + dist - 1.0) / i,
					(i + 4) % 3 + 'A');
			}
			printf("\tpSH[%d] = fTmp%c*fC%d;\n\tpSH[%d] = fTmp%c*fS%d;\n", index1 + dist, i % 3 + 'A', 1 - dist % 2, index1 - dist, i % 3 + 'A', 1 - dist % 2);
		}
	}
}