#include "Particle.h"

constexpr auto GRAVITY = 50;

Particle::Particle(Vector3 position, Vector3 velocity, float gravityEffect, float lifeLength, float rotation, float scale, bool split, float minHeight)
{
	this->position = position;
	this->velocity = velocity;
	this->gravityEffect = gravityEffect;
	this->lifeLength = lifeLength;
	this->rotation = rotation;
	this->scale = scale;
	this->split = split;
	this->minHeight = minHeight;
}

Matrix4 Particle::GetModelMatrix()
{
	Matrix4 model = Matrix4();

   	model = Matrix4::Scale(Vector3(scale, scale, scale)) * Matrix4::Rotation(rotation, Vector3(0, 0, 1)) * Matrix4::Translation(position) * model;
	
	return model;
}

bool Particle::Update(float dt)
{
	velocity.y += GRAVITY * gravityEffect * dt;
	position += velocity * dt;
	timeAlive += dt;
	return timeAlive < lifeLength && position.y > minHeight;
}