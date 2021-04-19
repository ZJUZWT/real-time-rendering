#pragma once

Camera camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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
		printf("GET LEFT BUTTON PRESS\n");
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		left_button_pressed = 0;
		printf("GET LEFT BUTTON RELEASE\n");
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		right_button_pressed = 1;
		right_button_pressed_just = 1;
		printf("GET RIGHT BUTTON PRESS\n");
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		right_button_pressed = 0;
		printf("GET RIGHT BUTTON RELEASE\n");
	}
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(yoffset);
}
//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//	if (key == GLFW_KEY_W && action == GLFW_REPEAT) {
//		ObjPos += glm::vec3(-0.05, 0, 0);
//		ObjVel = glm::vec3(-1, 0, 0);
//		printf("W\n");
//	}
//	if (key == GLFW_KEY_A && action == GLFW_REPEAT) {
//		ObjPos += glm::vec3(0, 0, 0.05);
//		ObjVel = glm::vec3(0, 0, 1);
//		printf("A\n");
//	}
//	if (key == GLFW_KEY_S && action == GLFW_REPEAT) {
//		ObjPos += glm::vec3(0.05, 0, 0);
//		ObjVel = glm::vec3(1, 0, 0);
//		printf("S\n");
//	}
//	if (key == GLFW_KEY_D && action == GLFW_REPEAT) {
//		ObjPos += glm::vec3(0, 0, -0.05);
//		ObjVel = glm::vec3(0, 0, -1);
//		printf("D\n");
//	}
//}