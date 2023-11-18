#pragma once
#include "OGLRenderer.h"
#include "Particle.h"
#include "HeightMap.h"

class ParticleSystem
{
public:
	ParticleSystem() { particlesPerSecond = 0; speed = 0; gravityCompliment = 0; lifeLength = 0; splittingParticles = false; };
	~ParticleSystem();
	void UpdateParticles(float dt);
	void UpdateParticles(float dt, HeightMap* heightmap);

	void SetParticlesPerSecond(float pps) { particlesPerSecond = pps; }
	void SetSpeed(float spd) { speed = spd; }
	void SetGravityComp(float gc) { gravityCompliment = gc; }
	void SetLifeLength(float ll) { lifeLength = ll; }
	void SetDirection(Vector3 dir) { direction = dir; }
	void SetSpawnQuad(Vector3 start, Vector3 end) { startPos = start; endPos = end; }
	void SetParticleSplit(bool split) { splittingParticles = split; }
	int GetParticleCount() { return particles.size(); }
	float* GetTranslations() { return translations; }
	void ClearParticleList() { particles.clear(); }
	
	std::vector<Particle*>::const_iterator GetParticleIteratorStart() { return particles.begin(); }
	std::vector<Particle*>::const_iterator GetParticleIteratorEnd() { return particles.end(); }

protected:
	void GenerateParticles(float dt);
	void GenerateParticles(float dt, HeightMap* heightmap);
	void EmitParticle();
	void EmitParticle(HeightMap* heightmap);
	void SplitParticle(Particle* p);
	
	float particlesPerSecond;
	float speed;
	float gravityCompliment;
	float lifeLength;
	Vector3 direction;
	Vector3 startPos;
	Vector3 endPos;
	bool splittingParticles;
	std::vector<Particle*> particles;
	float* translations = new float[45000];
};

