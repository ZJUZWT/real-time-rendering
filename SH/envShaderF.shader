#version 450 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D hdrSampler;

in vec3 localPos;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 p) {
	vec2 uv = vec2(atan(p.z, p.x), asin(p.y)); //������ʵ����һ�����ɣ���Ϊatan��ֵ����(-PI,PI)����acos��ֵ�������ģ�Ϊ�˵��͸������֣��������asin��Ϊpi/2����λ
	uv *= invAtan;
	uv += 0.5;
	return vec2(uv.x,1-uv.y);//����ȡ��Ϊ�˵���ͼƬ����v��y�Ĺ�ϵ
}
vec3 gamma(vec3 color) {
	return pow(color, vec3(1/2.2));
}
void main() {
	vec2 uv = SampleSphericalMap(normalize(localPos));

	//fragColor = vec4(localPos, 0.0);
	fragColor = vec4(texture(hdrSampler, uv).rgb,1.0);
}