#version 450 core

layout(location = 0) in vec4 aPos;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 worldPos;
out vec3 color;

uniform int coefNum;

uniform vec3 uColor;
uniform sampler1D prtCoef;
uniform sampler1D envCoef;

void main() {
	worldPos = aPos.xyz;
	gl_Position = P * V * M * vec4(aPos.xyz, 1.0);

	//color = uColor;

	color = vec3(0);
	int ID = int(aPos.w);
	for (int i = 0; i < coefNum; i++) {
		color.r += texelFetch(envCoef, i * 3 + 0, 0).x * texelFetch(prtCoef, ID * coefNum + i, 0).x;
		color.g += texelFetch(envCoef, i * 3 + 1, 0).x * texelFetch(prtCoef, ID * coefNum + i, 0).x;
		color.b += texelFetch(envCoef, i * 3 + 2, 0).x * texelFetch(prtCoef, ID * coefNum + i, 0).x;
	}
}