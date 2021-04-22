#version 450 core

layout(location = 0) out vec4 fragColor;

uniform vec3 albedo;
uniform float roughness;
uniform float metalness;

uniform int isGamma;
uniform vec3 camPos;
uniform samplerCube hdrSampler;

in vec3 worldPos;
in vec3 normal;

const float PI = 3.14159265359;

vec3 degamma(vec3 color) {
	return pow(color, vec3(2.2));
}
vec3 gamma(vec3 color) {
	return pow(color, vec3(1 / 2.2));
}
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 FresnelSphericalGaussian(float cosTheta, vec3 F0) {
	return F0 + (1 - F0) * exp2((-5.55473 * cosTheta - 6.98316) * cosTheta);
}
float GGX(float NdotH,float roughness) {
	float a = roughness * roughness;
	float NdotH2 = NdotH * NdotH;

	float nom = a * a;
	float denom = NdotH2 * (nom - 1) + 1;

	return nom / PI / denom / denom;
}
float Gsub(float cosTheta, float k) {
	return cosTheta / (cosTheta * (1 - k) + k);
}
float Gsmith(float NdotL, float NdotV, float k) {
	return Gsub(NdotL, k) * Gsub(NdotV, k);
}

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
vec3 hemisphereSample_uniform(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = 1.0 - u;
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
vec3 hemisphereSample(int i, int SampleNum) {
	vec2 hammersleyVec = Hammersley(i, SampleNum);
	//用vec3的结果直接在球面上实现均匀采样
	vec3 hemisphereVec = hemisphereSample_uniform(hammersleyVec.x, hammersleyVec.y);
	return hemisphereVec;
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
	vec3 N = normalize(normal);
	vec3 V = normalize(camPos - worldPos);
	//plane equation : N.x(x-worldPos.x) + N.y(y-worldPos.y) + N.z(z-worldPos.z) = 0
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metalness);

	const int SAMPLE_COUNT = 1024;

	//漫反射：主要贡献来源于光强，所以最好的策略是遍历卷积，而不是采样
	vec3 colorD = vec3(0);
	float sampleDelta = 0.125;
	float nrSamples = 0.0;
	vec3 C = albedo;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			vec3 L = tangentSample.x * tangent + tangentSample.y * bitangent + tangentSample.z * N;
			vec3 H = normalize(L + V);

			float HdotL = max(dot(L, H), 0.0);

			vec3 F = FresnelSphericalGaussian(HdotL, F0);
			vec3 kD = 1.0 - F; kD *= 1.0 - metalness;
			colorD += C * kD * texture(hdrSampler, L).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	colorD /= nrSamples / PI;
	//for (int i = 0; i < SAMPLE_COUNT; i++) {
	//	vec3 sampleL = hemisphereSample(i, SAMPLE_COUNT); 
	//	vec3 L = normalize(tangent * sampleL.x + bitangent * sampleL.y + N * sampleL.z);
	//	vec3 H = normalize(L + V);

	//	float HdotL = max(dot(L, H), 0.0);
	//	float NdotH = max(dot(N, H), 0.0);
	//	float NdotL = max(dot(N, L), 0.0);
	//	float NdotV = max(dot(N, V), 0.0);

	//	vec3 F = FresnelSphericalGaussian(HdotL, F0);
	//	vec3 kD = 1.0 - F; kD *= 1.0 - metalness;
	//	vec3 C = albedo;

	//	vec3 coefD = C / PI * kD * texture(hdrSampler, L).rgb * NdotL;

	//	colorD += coefD;
	//}
	//镜面反射
	vec3 colorS = vec3(0);
	if (roughness > 1.5) {
		sampleDelta = 0.125;
		nrSamples = 0.0;
		for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
			for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
				// spherical to cartesian (in tangent space)
				vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
				// tangent space to world
				vec3 L = tangentSample.x * tangent + tangentSample.y * bitangent + tangentSample.z * N;
				vec3 H = normalize(L + V);

				float HdotL = max(dot(L, H), 0.0);
				float NdotH = max(dot(N, H), 0.0);
				float NdotL = max(dot(N, L), 0.0);
				float NdotV = max(dot(N, V), 0.0);

				float D = GGX(NdotH, roughness);
				float G = max(Gsmith(NdotL, NdotV, roughness * roughness / 2), 0.0);
				vec3 F = FresnelSphericalGaussian(HdotL, F0);
				colorS += F * G * D / 4 / NdotV * texture(hdrSampler, L).rgb * sin(theta);
				//colorS += F * G * D / 4 / NdotV * texture(hdrSampler, L).rgb * sin(theta) * sampleDelta * sampleDelta;
				nrSamples++;
			}
		}
		colorS /= nrSamples / PI / PI;
	}
	else {
		for (int i = 0; i < SAMPLE_COUNT; i++) {
			vec2 Xi = Hammersley(i, SAMPLE_COUNT); //返回的第一个是分割数，在下面也就是代表了phi的分割，第二个数是一个随机数，在下面表示了roughness的影响程度
			vec3 H = ImportanceSampleGGX(Xi, N, roughness); //返回了一个在roughness影响下的微观表面向量
			vec3 L = normalize(2.0 * dot(V, H) * H - V);

			float HdotL = max(dot(L, H), 0.0);
			float NdotH = max(dot(N, H), 0.0);
			float NdotL = max(dot(N, L), 0.0);
			float NdotV = max(dot(N, V), 0.0);

			if (NdotL > 0) {
				vec3 F = FresnelSphericalGaussian(HdotL, F0);
				float D = 1;
				float G = max(Gsmith(NdotL, NdotV, roughness * roughness / 2), 0.0);

				// Incident light = SampleColor * NoL
                // Microfacet specular = D*G*F / (4*NoL*NoV)
                // pdf = D * NoH / (4 * VoH)
				vec3 coefS = F * G * HdotL / NdotV / NdotH * texture(hdrSampler, L).rgb;
				//vec3 coefS = F * G * D / 4 / NdotV * texture(hdrSampler, L).rgb;

				colorS += coefS;
			}
		}
		colorS /= SAMPLE_COUNT;
	}

	fragColor = vec4(colorS /*+ colorD*/, 1.0);
	//fragColor = vec4(degamma(color / SAMPLE_COUNT), 1.0);
	//fragColor = vec4(texture(hdrSampler, N).rgb, 1.0);
	if (isGamma == 1) fragColor = vec4(gamma(fragColor.rgb), 1.0);
}