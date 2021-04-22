#version 450 core

layout(location = 0) out vec4 fragColor;

uniform vec3 albedo;
uniform float roughness;
uniform float metalness;

uniform samplerCube prefilterMap;
uniform samplerCube irradianceMap;
uniform sampler2D brdfLUT;

uniform int isGamma;
uniform vec3 camPos;

in vec3 worldPos;
in vec3 normal;

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
	return F0 + (1 - F0) * pow(2, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

void main() {
	vec3 N = normal;
	vec3 V = normalize(camPos - worldPos);
	vec3 R = reflect(-V, N);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metalness);

	//vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	//vec3 F = FresnelSchlick(max(dot(N, V), 0.0), F0);
	vec3 F = FresnelSphericalGaussian(max(dot(N, V), 0.0), F0);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metalness;

	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	const float MAX_REFLECTION_LOD = 4.0;  
	vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 ambient = (kD * diffuse + specular);
	fragColor = vec4(specular, 1.0);

	if (isGamma == 1) fragColor = vec4(gamma(fragColor.rgb), 1.0);
}