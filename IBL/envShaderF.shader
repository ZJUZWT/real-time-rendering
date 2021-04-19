#version 450 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D hdrSampler;

in vec3 localPos;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}
vec3 gamma(vec3 color) {
	return pow(color, vec3(1/2.2));
}
void main() {
	vec2 uv = SampleSphericalMap(normalize(localPos));

	fragColor = vec4(texture(hdrSampler,uv).rgb,1.0);
}