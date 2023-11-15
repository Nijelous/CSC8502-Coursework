#pragma once
#include "../nclgl/Vector3.h"
#include "../nclgl/Vector2.h"
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

	void AddNode(Vector3 val, Vector2 pos) { 
		if (positionList.empty()) savedPosition = val; 
		positionList.push_back(val); lookAtList.push_back(pos); 
		initialPositionList.push_back(val); initialLookAtList.push_back(pos);
	}
	Vector3 GetNextNode() {
		positionList.push_back(positionList[0]);
		positionList.erase(positionList.begin());
		positionList.push_back(positionList[0]);
		positionList.erase(positionList.begin());
		lookAtList.push_back(lookAtList[0]);
		lookAtList.erase(lookAtList.begin());
		lookAtList.push_back(lookAtList[0]);
		lookAtList.erase(lookAtList.begin());
		count = 0;
		savedPosition = positionList[0];
		return positionList[0]; 
	}
	void ResetCamera() {
		freeCam = false; 
		positionList = initialPositionList; lookAtList = initialLookAtList; 
		count = 0; 
		position = positionList[0]; pitch = lookAtList[0].x; yaw = lookAtList[0].y;
		yawMinus = true;
		firstNodeFirst = false;
	}

protected:
	bool freeCam = false;
	float yaw;
	float pitch;
	float speed;
	Vector3 position;
	Vector3 savedPosition;
	Vector2 savedPitchYaw;
	int count = 0;
	std::vector<Vector3> positionList;
	std::vector<Vector2> lookAtList;
	std::vector<Vector3> initialPositionList;
	std::vector<Vector2> initialLookAtList;
	bool yawMinus = true;
	bool firstNodeFirst = false;
};

