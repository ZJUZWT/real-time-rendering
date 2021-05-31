#version 450 core

layout(location = 0) in vec4 aPos;

uniform int M;
uniform int SHOrder;
uniform int coefNum;

uniform vec3 lightColor;

layout(binding = 0, r32f) uniform imageBuffer uV;
layout(binding = 1, r32f) uniform imageBuffer uW;
layout(binding = 2, r32f) uniform imageBuffer uA;
layout(binding = 3, r32f) uniform imageBuffer uL;

float P(int l, float x) {
	if (l == 0) return 1;
	if (l == 1) return x;
	return x * (2 * l - 1) / l * P(l - 1, x) - 1.0 * (l - 1) / l * P(l - 2, x);
}

float C_CALC(int l, float a, float b, float gamma, float D, float B) {
	return
		((a * sin(gamma) - b * cos(gamma)) * P(l - 1, a * cos(gamma) + b * sin(gamma)) +
		b * P(l - 1, a) +
		(a * a + b * b - 1) * D +
		(l - 1) * B) / l;
}

const float PI = 3.1415926;

void main() {
	//vec3 w[10];

	//for (int e = 0; e < M; e++) {
	//	vec3 v = vec3(imageLoad(uV, e * 3 + 0).x, imageLoad(uV, e * 3 + 1).x, imageLoad(uV, e * 3 + 2).x);
	//	w[e] = v / length(v);
	//}
	//vec3 lambda[10];
	//vec3 mu[10];
	//float gamma[10];
	//for (int i = 0; i < M; i++) {
	//	vec3 omega1 = w[i];
	//	vec3 omega2 = w[(i + 1) % M];
	//	vec3 c12 = cross(omega1, omega2);

	//	mu[i] = normalize(c12);
	//	lambda[i] = normalize(cross(mu[i], omega1));
	//	gamma[i] = acos(dot(omega1, omega2));
	//}
	//float S[10];
	//float L[10][10];
	////S[0] = SolidAngle();
	//S[0] = 0;
	//for (int i = 0; i < M; i++) {
	//	vec3 omega1 = w[((i - 1) + M) % M];
	//	vec3 omega2 = w[i];
	//	vec3 omega3 = w[(i + 1) % M];
	//	S[0] += acos(dot(
	//		normalize(cross(omega2, omega1)), 
	//		normalize(cross(omega2, omega3))));
	//}
	//S[0] -= (M - 2) * PI;

	//for (int i = 0; i < SHOrder * 2 + 1 ; i++) {
	//	vec3 sampledOmega = vec3(imageLoad(uW, i * 3 + 0).x, imageLoad(uW, i * 3 + 1).x, imageLoad(uW, i * 3 + 2).x);
	//	S[1] = 0;
	//	float a[10];
	//	float b[10];
	//	float c[10];
	//	float B_[10];
	//	float B[10][10];
	//	float C[10][10];
	//	float D[10][10];

	//	for (int e = 0; e < M; e++) {
	//		a[e] = dot(sampledOmega, w[e]);
	//		b[e] = dot(sampledOmega, lambda[e]);
	//		c[e] = dot(sampledOmega, mu[e]);
	//		S[1] += c[e] * gamma[e] / 2; 
	//		B[0][e] = gamma[e];
	//		B[1][e] = a[e] * sin(gamma[e]) - b[e] * cos(gamma[e]) + b[e];
	//		D[0][e] = 0;
	//		D[1][e] = gamma[e];

	//		B_[0] += B[0][e] * c[e];
	//		B_[1] += B[1][e] * c[e];
	//	}

	//	for (int l = 2; l <= SHOrder; l++) {
	//		B_[l] = 0;
	//		for (int e = 0; e < M; e++) {
	//			C[l - 1][e] = C_CALC(l, a[e], b[e], gamma[e], D[l - 1][e], B[l - 2][e]);
	//			B[l][e] = C[l - 1][e] * (2 * l - 1) / l - B[l - 2][e] * (l - 1) / l;
	//			B_[l] = B_[l] + c[e] * B[l][e];
	//			D[l][e] = (2 * l - 1) * B[l - 1][e] + D[l - 2][e];
	//		}
	//		S[l] = B_[l - 1] * (2 * l - 1) / l / (l + 1) + S[l - 2] * (l - 2) * (l - 1) / l / (l + 1);
	//	}

	//	for (int l = 0; l <= SHOrder; l++) 
	//		L[l][i] = sqrt((2 * l + 1) / PI / 4) * S[l];
	//}
	//int index = 0;
	//for (int l = 0; l <= SHOrder; l++) {
	//	for (int m = -l; m <= l; m++) {
	//		float tempL = 0;
	//		for (int lobe = 0; lobe < 2 * l + 1; lobe++) {
	//			tempL += imageLoad(uA, index + (m + l) * (2 * l + 1) + lobe) * L[l][lobe];
	//		}
	//		//imageStore(uL, l * l + m + l + int(aPos.w) * coefNum, vec4(tempL));
	//		imageStore(uL, l * l + m + l + int(aPos.w) * coefNum, 1);
	//	}
	//	index += (2 * l + 1) * (2 * l + 1);
	//}

	int index = 0;
	for (int l = 0; l <= SHOrder; l++) {
		for (int m = -l; m <= l; m++) {
			float tempL = 0;
			//for (int lobe = 0; lobe < 2 * l + 1; lobe++) {
			//	tempL += imageLoad(uA, index + (m + l) * (2 * l + 1) + lobe) * L[l][lobe];
			//}
			imageStore(uL, l * l + m + l + int(aPos.w) * coefNum, vec4(1));
		}
		index += (2 * l + 1) * (2 * l + 1);
	}
}