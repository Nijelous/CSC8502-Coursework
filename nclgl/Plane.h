#pragma once
#include "Vector3.h"
class Plane
{
public:
	Plane(void) {};
	Plane(const Vector3& normal, float distance, bool normalise = false);
	~Plane(void) {};

	Vector3 GetNormal() const { return normal; }
	void SetNormal(const Vector3& normal) { this->normal = normal; }

	float GetDistance() const { return distance; }
	void SetDistance(float dist) { distance = dist; }

	bool SphereInPlane(const Vector3& position, float radius) const;

protected:
	Vector3 normal;
	float distance;
};

