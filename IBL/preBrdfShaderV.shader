#version 450 core

layout(location = 0) in vec3 modelPos;

out vec3 worldPos;

void main() {
	worldPos = modelPos;
	gl_Position = vec4(modelPos, 1.0);
}