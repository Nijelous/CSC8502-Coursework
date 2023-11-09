#include "Camera.h"
#include "../nclgl/Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt)
{
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float timeSpeed = speed * dt;

	if (freeCam) {
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
			position += forward * timeSpeed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
			position -= forward * timeSpeed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
			position -= right * timeSpeed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) {
			position += right * timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) {
			position.y += timeSpeed;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
			position.y -= timeSpeed;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C)) {
			freeCam = false;
			position = savedPosition;
		}
	}
	else {
		position += (positionList[1] - positionList[0]) / (100.0f);
		count++;
		if (count == 100) {
			positionList.push_back(positionList[0]);
			positionList.erase(positionList.begin());
			count = 0;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C)) {
			freeCam = true;
			savedPosition = position;
		}
	}
}

Matrix4 Camera::BuildViewMatrix()
{
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
}
