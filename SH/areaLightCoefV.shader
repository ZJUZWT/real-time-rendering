#version core 450

layout(location = 0) in vec3 aPos;

uniform sampler1D uV;
uniform sampler1D uW;
uniform sampler1D uA;
uniform int M;
uniform int SHOrder;
uniform int coefNum;

float P(int l, float x) {
	return x * (2 * l - 1) / l * P(l - 1, x) - 1.0 * (l - 1) / l * P(l - 2, x);
}

float C(int l, float a, float b, float gamma, float D, float B) {
	return
		((a * sin(gamma) - b * cos(gamma)) * P(l - 1, a * cos(gamma) + b * sin(gamma)) +
		b * P(l - 1, a) +
		(a * a + b * b - 1) * D +
		(l - 1) * B) / l;
}

const float PI = 3.1415926;

void main() {
	vec3 w[10];

	for (int e = 0; e < M; e++) {
		vec3 v = vec3(texelFetch(uV, e * 3 + 0, 0).x, texelFetch(uV, e * 3 + 1, 0).x, texelFetch(uV, e * 3 + 2, 0).x);
		w[e] = v / length(v);
	}
	vec3 lambda[10];
	vec3 mu[10];
	float gamma[10];
	for (int i = 0; i < M; i++) {
		vec3 omega1 = w[i];
		vec3 omega2 = w[(i + 1) % M];
		vec3 c12 = cross(omega1, omega2);

		lambda[i] = cross(c12 / length(c12), omega1);
		mu[i] = cross(omega1, lambda[i]);
		gamma[i] = acos(dot(omega1, omega2));
	}
	float S[10];
	//S[0] = SolidAngle();
	S[0] = 0;
	for (int i = 0; i < M; i++) {
		vec3 omega1 = w[((i - 1) + M) % M];
		vec3 omega2 = w[i];
		vec3 omega3 = w[(i + 1) % M];
		S[0] += acos(dot(cross(omega1, omega2), cross(omega2, omega3))) / (length(cross(omega1, omega2)) * length(cross(omega2, omega3)));
	}
	S[0] -= (M - 2) * PI;


	for (int i = 0; i < SHOrder * 2 + 1 ; i++) {
		vec3 sampledOmega = vec3(texelFetch(uW, i * 3 + 0, 0).x, texelFetch(uW, i * 3 + 1, 0).x, texelFetch(uW, i * 3 + 2, 0).x);
		S[1] = 0;
		float a[10];
		float b[10];
		float c[10];
		float B[10][10];
		float C[10][10];
		float D[10][10];

		for (int e = 0; e < M; e++) {
			a[e] = dot(sampledOmega, w[e]);
			b[e] = dot(sampledOmega, lambda[e]);
			c[e] = dot(sampledOmega, mu[e]);
			S[1] += c[e] * gamma[e] / 2; 
			B[0][e] = gamma[e];
			B[1][e] = a[e] * sin(gamma[e]) - b[e] * cos(gamma[e]) + b[e];
			D[0][e] = 0;
			D[1][e] = gamma[e];
			D[2][e] = 3 * B[1][e];
		}

		for (int l = 2; l <= SHOrder; l++) {
			B_[l] = 0;
			for (int e = 0; e < M; e++) {
				C[l - 1][e] = C(l, a[e], b[e], gamma[e], D[l - 1][e], B[l - 2][e]);
				B[l][e] = C[l - 1][e] * (2 * l - 1) / l - B[l - 2][e] * (l - 1);
				B_[l] = B_[l] + c[e] * B[l][e];
				D[l][e] = (2 * l - 1)B[l - 1][e] + D[l - 2][e];
			}
			S[l] = B[l - 1] * (2 * l - 1) / l / (l + 1) + S[l - 2] * (l - 2) * (l - 1) / l / (l + 1);
			L[l][sampledOmega] = sqrt((2l + 1) / PI / 4) * S[l];
		}
	}
	
	for (int l = 0; l <= SHOrder; l++) 
		for (int m = -l; m <= l; m++) {
			int id = l * l + m + l;
			L_[id] = 0;
			for (int lobe = 0; lobe < 2 * l + 1; lobe++) {
				vec3 sampledOmega = vec3(texelFetch(uW, lobe * 3 + 0, 0).x, texelFetch(uW, lobe * 3 + 1, 0).x, texelFetch(uW, lobe * 3 + 2, 0).x);
				L_[id] = alpha[id][lobe] * L[l][sampledOmega];
			}
		}
}