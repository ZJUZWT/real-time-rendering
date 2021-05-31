 #pragma once

//#define SHOW_MAT
#include <iomanip>
#include "SHStrutct.h"

class MatrixList {
public:
	MatrixList() {
		int offset = 0;
		for (int p = 0; p <= SH_ORDER; p++) offset += p * p * 4 + p * 4 + 1;
		mat = new Float[offset];
	}
	Float& at(int i, int j, int k) {
		int offset = 0;
		for (int p = 0; p < i; p++) offset += p * p * 4 + p * 4 + 1;
		j += i; k += i;
		return mat[offset +j*(i*2+1)+k];
	}
	Float at(int i, int j, int k) const {
		int offset = 0;
		for (int p = 0; p < i; p++) offset += p * p * 4 + p * 4 + 1;
		j += i; k += i;
		return mat[offset + j * (i * 2 + 1) + k];
	}
private:
	Float* mat;
};

void Ivanic_paper_uvw(const int l, const int m1, const int m2, Float& u, Float& v, Float& w) {
	Float delta = m1 == 0 ? 1 : 0;
	Float absM1 = fabs(m1);
	Float absM2 = fabs(m2);
	Float denominator;
	Float l_ = l;
	if (absM2 < l) denominator = 1.0 / (l_ + m2) / (l_ - m2); else denominator = 1.0 / (2 * l_) / (2 * l_ - 1);

	u = sqrt((l_ + m1) * (l_ - m1) * denominator);
	v = 0.5 * sqrt((1 + delta) * (l + absM1 - 1) * (l + absM1) * denominator) * (1 - 2 * delta);
	w = -0.5 * sqrt((l - absM1 - 1) * (l - absM1) * denominator) * (1 - delta);
}
Float C(int m, int n) {
	Float res = 1;
	for (int i = 1; i <= n; i++) res *= i;
	for (int i = 1; i <= m; i++) res /= i;
	for (int i = 1; i <= n - m; i++) res /= i;
	return res;
}
Float Ivanic_paper_T(const int k, const int l, const int m1, const int m2, const int l_, const int m1_, const int m2_, const MatrixList* R) {
	Float res = 0;
	for (int i = 0; i <= k; i++) {
		Float combination = C(i, k);
		res += combination * R[i].at(l, m1, m2) * R[k - i].at(l_, m1_, m2_);
	}
	return res;
}
Float Ivanic_paper_P(const int k, const int l, const int m1, const int m2, const int i, const MatrixList* R) {
	if (abs(m2) < l) return Ivanic_paper_T(k, 1, i, 0, l - 1, m1, m2, R);
	else if (m2 == l) return Ivanic_paper_T(k, 1, i, 1, l - 1, m1, l - 1, R) - Ivanic_paper_T(k, 1, i, -1, l - 1, m1, -l + 1, R);
	else if (m2 == -l)return Ivanic_paper_T(k, 1, i, 1, l - 1, m1, -l + 1, R) + Ivanic_paper_T(k, 1, i, -1, l - 1, m1, l - 1, R);
}
Float Ivanic_paper_W(const int k, const int l, const int m1, const int m2, const MatrixList* R) {
	if (m1 > 0) return Ivanic_paper_P(k, l, m1 + 1, m2, 1, R) + Ivanic_paper_P(k, l, -m1 - 1, m2, -1, R);
	else return Ivanic_paper_P(k, l, m1 - 1, m2, 1, R) - Ivanic_paper_P(k, l, -m1 + 1, m2, -1, R);
}
Float Ivanic_paper_V(const int k, const int l, const int m1, const int m2, const MatrixList* R) {
	if (m1 > 1) return Ivanic_paper_P(k, l, m1 - 1, m2, 1, R) - Ivanic_paper_P(k, l, -m1 + 1, m2, -1, R);
	else if (m1 == 1) return sqrt(2) * Ivanic_paper_P(k, l, 0, m2, 1, R);
	else if (m1 == 0) return Ivanic_paper_P(k, l, 1, m2, 1, R) + Ivanic_paper_P(k, l, -1, m2, -1, R);
	else if (m1 == -1 ) return sqrt(2) * Ivanic_paper_P(k, l, 0, m2, -1, R);
	else return Ivanic_paper_P(k, l, -m1-1, m2, -1, R) + Ivanic_paper_P(k, l, m1+1, m2, 1, R);
}
Float Ivanic_paper_U(const int k, const int l, const int m1, const int m2, const MatrixList* R) {
	return Ivanic_paper_P(k, l, m1, m2, 0, R);
}

const int DY_ORDER = 2;
class SHRotateMat {
public:
	SHRotateMat() { constructMat(); }
	//矩阵0的泰勒展开
	void constructMat() {
		MatrixList* R = new MatrixList[DY_ORDER + 1];

		R[0].at(0, 0, 0) = 1;
		R[0].at(1, -1, -1) = 1;
		R[0].at(1, -1, 0) = 0;
		R[0].at(1, -1, 1) = 0;
		R[0].at(1, 0, -1) = 0;
		R[0].at(1, 0, 0) = 1;
		R[0].at(1, 0, 1) = 0;
		R[0].at(1, 1, -1) = 0;
		R[0].at(1, 1, 0) = 0;
		R[0].at(1, 1, 1) = 1;

		R[1].at(0, 0, 0) = 0;
		R[1].at(1, -1, -1) = 0;
		R[1].at(1, -1, 0) = 0;
		R[1].at(1, -1, 1) = 0;
		R[1].at(1, 0, -1) = 0;
		R[1].at(1, 0, 0) = 0;
		R[1].at(1, 0, 1) = -1;
		R[1].at(1, 1, -1) = 0;
		R[1].at(1, 1, 0) = 1;
		R[1].at(1, 1, 1) = 0;

		R[2].at(0, 0, 0) = 0;
		R[2].at(1, -1, -1) = 0;
		R[2].at(1, -1, 0) = 0;
		R[2].at(1, -1, 1) = 0;
		R[2].at(1, 0, -1) = 0;
		R[2].at(1, 0, 0) = -1;
		R[2].at(1, 0, 1) = 0;
		R[2].at(1, 1, -1) = 0;
		R[2].at(1, 1, 0) = 0;
		R[2].at(1, 1, 1) = -1;

		int countDDY = 0;
		int countDY = 0;

		for (int l = 2; l < SH_ORDER; l++) {
			for (int k = 0; k <= DY_ORDER; k++)
				for (int m1 = -l; m1 <= l; m1++)
					for (int m2 = -l; m2 <= l; m2++) {
						Float u, v, w;
						Float U, V, W;
						Ivanic_paper_uvw(l, m1, m2, u, v, w);
						U = Ivanic_paper_U(k, l, m1, m2, R);
						V = Ivanic_paper_V(k, l, m1, m2, R);
						W = Ivanic_paper_W(k, l, m1, m2, R);
						R[k].at(l, m1, m2) = u * U + v * V + w * W;
					}
		}
		//一阶导数
		for (int l = 0; l < SH_ORDER; l++) {
			for (int m = -l + 1; m <= l; m++) dySubDiag[countDY++] = R[1].at(l, m, m - 1);
			dySubDiag[countDY++] = 0;
		}
		//二阶导数
		for (int l = 0; l < SH_ORDER; l++)
			for (int m = -l; m <= l; m++) ddyDiag[countDDY++] = R[2].at(l, m, m);


#ifdef SHOW_MAT
		std::cout << std::setprecision(3);

		for (int k = 0; k <= DY_ORDER; k++)
			for (int l = 0; l <= SH_ORDER; l++) {
				std::cout << "R[" << k << "]" << "[" << l << "]" << std::endl;
				for (int m1 = -l; m1 <= l; m1++) {
					for (int m2 = -l; m2 <= l; m2++)
						std::cout << R[k].at(l, m1, m2) << "\t\t";
					std::cout << "\n";
				}
		}

		std::cout << "DY:" << std::endl;
		for (int i = 0; i < countDY; i++) std::cout << dySubDiag[i] << std::endl;
		std::cout << "DDY:" << std::endl;
		for (int i = 0; i < countDDY; i++) std::cout << ddyDiag[i] << std::endl;
#endif // SHOW_MAT
		delete[] R;
	}
	Float* getDdyDiag() { return ddyDiag; }
	Float* getDySubDiag() { return dySubDiag; }
private:
	//泰勒展开的结果，是常数矩阵
	Float dySubDiag[SH_COEFNUM];
	Float ddyDiag[SH_COEFNUM];
};