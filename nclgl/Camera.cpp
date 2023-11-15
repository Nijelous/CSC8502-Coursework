#include "Camera.h"
#include "../nclgl/Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt)
{
	if (freeCam) {
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
			pitch = savedPitchYaw.x;
			yaw = savedPitchYaw.y;
		}
	}
	else {
		if (count == 0) {
			yawMinus = !(abs(lookAtList[1].y - yaw) > 180);
			firstNodeFirst = lookAtList[1].y < yaw;
		}
		position += (positionList[1] - positionList[0]) / (400.0f);
		pitch += ((lookAtList[1].x - lookAtList[0].x) / 400.0f);
		if(yawMinus) yaw += ((lookAtList[1].y - lookAtList[0].y) / 400.0f);
		else {
			if(firstNodeFirst) yaw += (360.0f - (lookAtList[0].y - lookAtList[1].y)) / 400.0f;
			else yaw -= (360.0f - (lookAtList[1].y - lookAtList[0].y)) / 400.0f;
			if (yaw < 0) {
				yaw += 360.0f;
			}
			if (yaw > 360.0f) {
				yaw -= 360.0f;
			}
		}
		count++;
		if (count == 400) {
			positionList.push_back(positionList[0]);
			positionList.erase(positionList.begin());
			lookAtList.push_back(lookAtList[0]);
			lookAtList.erase(lookAtList.begin());
			count = 0;
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C)) {
			freeCam = true;
			savedPosition = position;
			savedPitchYaw = Vector2(pitch, yaw);
		}
	}
}

Matrix4 Camera::BuildViewMatrix()
{
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
}
