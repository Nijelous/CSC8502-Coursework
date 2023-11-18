#include "ParticleSystem.h"

ParticleSystem::~ParticleSystem()
{
	for (unsigned int i = 0; i < particles.size(); ++i) {
		delete particles[i];
	}
}

void ParticleSystem::UpdateParticles(float dt)
{
	int count = 0;
	for (int i = 0; i < particles.size(); i++) {
		if (!particles[i]->Update(dt)) {
			if (particles[i]->GetSplit()) {
				SplitParticle(particles[i]);
			}
			delete particles[i];
			particles.erase(std::remove(particles.begin(), particles.end(), particles[i]), particles.end());
			i--;
		}
		else {
			translations[count] = particles[i]->GetPosition().x;
			translations[count+1] = particles[i]->GetPosition().y;
			translations[count+2] = particles[i]->GetPosition().z;
			count += 3;
		}
	}
	GenerateParticles(dt);
}

void ParticleSystem::UpdateParticles(float dt, HeightMap* heightmap)
{
	int count = 0;
	for (int i = 0; i < particles.size(); i++) {
		if (!particles[i]->Update(dt*10)) {
			if (particles[i]->GetSplit()) {
				SplitParticle(particles[i]);
			}
			delete particles[i];
			particles.erase(std::remove(particles.begin(), particles.end(), particles[i]), particles.end());
			i--;
		}
		else {
			translations[count++] = particles[i]->GetPosition().x;
			translations[count++] = particles[i]->GetPosition().y;
			translations[count++] = particles[i]->GetPosition().z;
		}
	}
	GenerateParticles(dt*10, heightmap);
}

void ParticleSystem::GenerateParticles(float dt)
{
	int particlesToCreate = particlesPerSecond * dt;
	for (int i = 0; i < particlesToCreate; i++) {
		EmitParticle();
	}
}

void ParticleSystem::GenerateParticles(float dt, HeightMap* heightmap)
{
	int particlesToCreate = particlesPerSecond * dt;
	for (int i = 0; i < particlesToCreate; i++) {
		EmitParticle(heightmap);
	}
}

void ParticleSystem::EmitParticle()
{
	Vector3 velocity = direction;
	velocity.Normalise();
	float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.x - startPos.x));
	float y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.y - startPos.y));
	float z = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.z - startPos.z));
	particles.push_back(new Particle(Vector3(startPos.x + x, startPos.y + y, startPos.z + z), 
		velocity * speed, gravityCompliment * velocity.y, lifeLength, 0, 1, splittingParticles));
}

void ParticleSystem::EmitParticle(HeightMap* heightmap)
{
	Vector3 velocity = direction;
	velocity.Normalise();
	float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.x - startPos.x));
	float y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.y - startPos.y));
	float z = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (endPos.z - startPos.z));
	particles.push_back(new Particle(Vector3(startPos.x + x, startPos.y + y, startPos.z + z),
		velocity * speed, gravityCompliment * velocity.y, lifeLength, 0, 1, splittingParticles, std::max(heightmap->GetHeightAt(x, z), 97.5f)));
}

void ParticleSystem::SplitParticle(Particle* p)
{
	particles.push_back(new Particle(p->GetPosition(), Vector3(1, 1, 1) * speed, -gravityCompliment, 4, 0, 1, false));
	particles.push_back(new Particle(p->GetPosition(), Vector3(-1, 1, 1) * speed, -gravityCompliment, 4, 0, 1, false));
	particles.push_back(new Particle(p->GetPosition(), Vector3(1, 1, -1) * speed, -gravityCompliment, 4, 0, 1, false));
	particles.push_back(new Particle(p->GetPosition(), Vector3(-1, 1, -1) * speed, -gravityCompliment, 4, 0, 1, false));
}
