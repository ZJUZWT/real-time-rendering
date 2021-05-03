#version 450 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D hdrSampler;

in vec3 localPos;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 p) {
	vec2 uv = vec2(atan(p.z, p.x), asin(p.y)); //这里其实用了一个技巧，因为atan的值域是(-PI,PI)，而acos的值域是正的，为了调和负数部分，这里就用asin作为pi/2的相位
	uv *= invAtan;
	uv += 0.5;
	return vec2(uv.x,1-uv.y);//这里取反为了调和图片像素v与y的关系
}
vec3 gamma(vec3 color) {
	return pow(color, vec3(1/2.2));
}
void main() {
	vec2 uv = SampleSphericalMap(normalize(localPos));

	//fragColor = vec4(localPos, 0.0);
	fragColor = vec4(texture(hdrSampler, uv).rgb,1.0);
}