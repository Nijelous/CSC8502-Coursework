#pragma once
#include "Vector3.h"
#include "Matrix4.h"

class Particle
{
public:
	Particle(Vector3 position, Vector3 velocity, float gravityEffect, float lifeLength, float rotation, float scale, bool split, float minHeight = -100000);
	~Particle(){}

	Vector3 GetPosition() { return position; }

	float GetRotation() { return rotation; }

	float GetScale() { return scale; }

	bool GetSplit() { return split; }

	float getMinHeight() { return minHeight; }
	
	Matrix4 GetModelMatrix();

	bool Update(float dt);

protected:
	Vector3 position;
	Vector3 velocity;
	float gravityEffect;
	float lifeLength;
	float rotation;
	float scale;
	float minHeight;

	float timeAlive = 0;

	bool split;
};

