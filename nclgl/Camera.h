#pragma once
#include "../nclgl/Vector3.h"
#include "../nclgl/Matrix4.h"
#include <vector>

class Camera
{
public:
	Camera(void) {
		yaw = 0.0f;
		pitch = 0.0f;
		speed = 30.0f;
	};

	Camera(float pitch, float yaw, Vector3 position) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;
		speed = 200.0f;
	}

	~Camera(void) {};

	void UpdateCamera(float dt = 1.0f);

	Matrix4 BuildViewMatrix();

	Vector3 GetPosition() const { return position; }
	void SetPosition(Vector3 val) { position = val; }

	float GetYaw() const { return yaw; }
	void SetYaw(float y) { yaw = y; }

	float GetPitch() const { return pitch; }
	void SetPitch(float p) { pitch = p; }

	void SetSpeed(float s) { speed = s; }

	void AddNode(Vector3 val) { if(positionList.empty()) savedPosition = val; positionList.push_back(val); }

protected:
	bool freeCam = true;
	float yaw;
	float pitch;
	float speed;
	Vector3 position;
	Vector3 savedPosition;
	int count = 0;
	std::vector<Vector3> positionList;
};

