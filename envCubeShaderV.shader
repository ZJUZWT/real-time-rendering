#version 450 core

layout(location = 0) in vec3 modelPos;
layout(location = 1) in vec2 uv;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 localPos;

void main() {
	localPos = (M * vec4(modelPos, 1.0)).xyz; 

	gl_Position = P * mat4(mat3(V)) * vec4(modelPos, 1.0);
	gl_Position = gl_Position.xyww;
}