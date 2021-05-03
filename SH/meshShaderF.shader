#version 450 core

layout(location = 0) out vec4 fragColor;

in vec3 worldPos;
in vec3 color;

vec3 gamma(vec3 color) {
	return pow(color, vec3(1 / 2.2));
}

void main() {
	fragColor = vec4(gamma(color), 1.0);
}