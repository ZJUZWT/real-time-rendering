#pragma once

Camera camera(glm::vec3(0.1f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

float rotateSpeed = 0;
float rotateAngle = 0;
glm::vec3 rotatedAxis(0, 1, 0);
glm::mat4 rotatedMatrix(glm::identity<glm::mat4>());

bool backGroundShading = true;

double lastX, lastY;
int left_button_pressed = 0;
int left_button_pressed_just = 0;
int right_button_pressed = 0;
int right_button_pressed_just = 0;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (left_button_pressed) {
		if (left_button_pressed_just) {
			lastX = xpos;
			lastY = ypos;
			left_button_pressed_just = 0;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		float sensitivity = 0.5;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		camera.ProcessLeftMouseMovement(-xoffset, -yoffset);
	}

	if (right_button_pressed) {
		if (right_button_pressed_just) {
			lastX = xpos;
			lastY = ypos;
			right_button_pressed_just = 0;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		float sensitivity = 0.5;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		camera.ProcessRightMouseMovement(-xoffset, -yoffset);
	}
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		left_button_pressed = 1;
		left_button_pressed_just = 1;
		//printf("GET LEFT BUTTON PRESS\n");
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		left_button_pressed = 0;
		//printf("GET LEFT BUTTON RELEASE\n");
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		right_button_pressed = 1;
		right_button_pressed_just = 1;
		//printf("GET RIGHT BUTTON PRESS\n");
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		right_button_pressed = 0;
		//printf("GET RIGHT BUTTON RELEASE\n");
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		right_button_pressed = 0;
		//printf("GET RIGHT BUTTON RELEASE\n");
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		right_button_pressed = 0;
		//printf("GET RIGHT BUTTON RELEASE\n");
	}
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(yoffset);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		rotateSpeed += 0.1;
		//printf("Rotate SPEED : %f\n", rotateSpeed);
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		rotateSpeed -= 0.1;
		if (rotateSpeed < 0) rotateSpeed = 0;
		//printf("Rotate SPEED : %f\n", rotateSpeed);
	}
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		rotatedAxis = glm::vec3(rotatedAxis.y, rotatedAxis.z, rotatedAxis.x);
		rotatedMatrix = glm::rotate(glm::identity<glm::mat4>(), rotateAngle, rotatedAxis);
	}
	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		backGroundShading != backGroundShading;
	}
}