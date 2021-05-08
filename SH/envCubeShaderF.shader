#version 450 core

layout(location = 0) out vec4 fragColor;

uniform samplerCube hdrSampler;

uniform int isGamma;

in vec3 localPos;

vec3 gamma(vec3 color) {
	return pow(color, vec3(1 / 2.2));
}
void main() {
	fragColor = vec4(texture(hdrSampler, localPos).rgb, 1.0);
	if (isGamma == 1) fragColor = vec4(gamma(fragColor.xyz), 1.0);
}