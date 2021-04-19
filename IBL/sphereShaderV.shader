#version 450 core

layout(location = 0) in vec3 modelPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normalIn;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 worldPos;
out vec3 normal;

void main() {
	worldPos = (M * vec4(modelPos, 1.0)).xyz;
	normal = normalIn;
	gl_Position = P * V * M * vec4(modelPos, 1.0);
}