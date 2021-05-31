#pragma once
#include <cmath>
//#define HighP

#ifdef HighP
typedef double Float;
#else 
typedef float Float;
#endif

//const double PI = 4.0 * atan(1.0);
const int SH_ORDER = 10;
const int SH_COEFNUM = SH_ORDER * SH_ORDER;

//定义系数数组
class SHCoef {
public:
	SHCoef() { val = new Float[SH_COEFNUM]{ 0 }; };
	SHCoef(int shNum) { val = new Float[(shNum + 1) * (shNum + 1)]{ 0 }; }
	Float& operator[](int i) { return val[i]; }
	const Float& operator[](int i) const { return val[i]; }
	Float* getPtr() { return val; }
private:
	Float *val;
};
//定义颜色分量数组
class SHColor {
public:
	SHColor() {};
	SHColor(int shNum) { for (int i = 0; i < 3; i++) color[i] = SHCoef(shNum); }
	SHCoef& operator[](int i) { return color[i]; }
	const SHCoef& operator[](int i) const { return color[i]; }
private:
	SHCoef color[3];
};
