#version 450 core

layout(location = 0) in vec4 aPos;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 color;

uniform int coefNum;
uniform int dispNum;

uniform vec3 uColor;
uniform sampler1D envCoef;
//uniform sampler2D prtCoef;

layout(binding = 0, r32f) uniform imageBuffer prtCoef;
layout(binding = 1, r32f) uniform imageBuffer uL;

void main() {
	gl_Position = P * V * /*M * */vec4(aPos.xyz, 1.0);

	color = vec3(0);
	int ID = int(aPos.w);
	for (int i = 0; i < coefNum; i++) {
	//for (int i = 0; i < dispNum; i++) {
		//color.r += texelFetch(envCoef, i * 3 + 0, 0).x * imageLoad(prtCoef, i + ID * coefNum).x;
		//color.g += texelFetch(envCoef, i * 3 + 1, 0).x * imageLoad(prtCoef, i + ID * coefNum).x;
		//color.b += texelFetch(envCoef, i * 3 + 2, 0).x * imageLoad(prtCoef, i + ID * coefNum).x;
		//color.r += imageLoad(uL, (i + ID * coefNum) * 3 + 0).x * imageLoad(prtCoef, i + ID * coefNum).x;
		//color.g += imageLoad(uL, (i + ID * coefNum) * 3 + 1).x * imageLoad(prtCoef, i + ID * coefNum).x;
		//color.b += imageLoad(uL, (i + ID * coefNum) * 3 + 2).x * imageLoad(prtCoef, i + ID * coefNum).x;
		color.r += (texelFetch(envCoef, i * 3 + 0, 0).x + imageLoad(uL, (i + ID * coefNum) * 3 + 0).x) * imageLoad(prtCoef, i + ID * coefNum).x;
		color.g += (texelFetch(envCoef, i * 3 + 1, 0).x + imageLoad(uL, (i + ID * coefNum) * 3 + 1).x) * imageLoad(prtCoef, i + ID * coefNum).x;
		color.b += (texelFetch(envCoef, i * 3 + 2, 0).x + imageLoad(uL, (i + ID * coefNum) * 3 + 2).x) * imageLoad(prtCoef, i + ID * coefNum).x;
	}
	//color = vec3(color.r, -color.r, 0);
}