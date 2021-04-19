#version 450 core

layout(location = 0) out vec4 fragColor;

uniform samplerCube hdrSampler;
uniform float roughness;

in vec3 localPos;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N) {
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {//这个部分不太懂
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y)); //Xi.y 提供了一个roughness对采样的影响程度，随机的Xi.y不一样，roughness影响程度不一样
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H; //这是生成的半球面空间的向量
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	//我们需要将半球面空间向量转化到世界坐标系的值

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main() {
	//假设出射向量V是和宏观表面相同的
	vec3 N = normalize(localPos);
	vec3 V = N;
	vec3 R = V;

	vec3 prefilter = vec3(0.0);
	const uint SAMPLE_COUNT = 32768u;
	float totalWeight = 0.0;
	for (uint i = 0u; i < SAMPLE_COUNT; i++) {
		vec2 Xi = Hammersley(i, SAMPLE_COUNT); //返回的第一个是分割数，在下面也就是代表了phi的分割，第二个数是一个随机数，在下面表示了roughness的影响程度
		vec3 H = ImportanceSampleGGX(Xi, N, roughness); //返回了一个在roughness影响下的微观表面向量
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0) {
			prefilter += texture(hdrSampler, L).rgb * NdotL;
			totalWeight += NdotL;
		}
	}
	prefilter = prefilter / totalWeight;

	fragColor = vec4(prefilter, 1.0);
}