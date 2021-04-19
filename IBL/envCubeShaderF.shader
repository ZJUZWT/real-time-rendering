#version 450 core

layout(location = 0) out vec4 fragColor;

uniform samplerCube hdrSampler;

in vec3 localPos;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec3 gamma(vec3 color) {
	return pow(color, vec3(1 / 2.2));
}
void main() {
	fragColor = vec4(texture(hdrSampler, localPos).rgb, 1.0);
	//fragColor = vec4(0.5);

	fragColor = vec4(gamma(fragColor.xyz), 1.0);
}