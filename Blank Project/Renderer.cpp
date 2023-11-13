#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include <algorithm>

#define SHADOWSIZE 2048

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	asteroid = Mesh::LoadFromMeshFile("Rock.msh");
	tree = Mesh::LoadFromMeshFile("Tree.msh");

	heightMap = new HeightMap(TEXTUREDIR"swampHeightmap.png");

	planetTexture = SOIL_load_OGL_texture(TEXTUREDIR"Planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"PlanetDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	asteroidTexture = SOIL_load_OGL_texture(TEXTUREDIR"Rock.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	asteroidBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"RockDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	surfaceTexture = SOIL_load_OGL_texture(TEXTUREDIR"Swampland.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	surfaceBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"SwamplandDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	barkTexture = SOIL_load_OGL_texture(TEXTUREDIR"Tree Bark.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	barkBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"Tree BarkDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"swampWater.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	spaceSkybox = SOIL_load_OGL_cubemap(
		TEXTUREDIR"SkyBlue_right.png", TEXTUREDIR"SkyBlue_left.png",
		TEXTUREDIR"SkyBlue_top.png", TEXTUREDIR"SkyBlue_bottom.png",
		TEXTUREDIR"SkyBlue_front.png", TEXTUREDIR"SkyBlue_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	landDaySkybox = SOIL_load_OGL_cubemap(
		TEXTUREDIR"GreenSky_right.jpg", TEXTUREDIR"GreenSky_left.jpg",
		TEXTUREDIR"GreenSky_up.jpg", TEXTUREDIR"GreenSky_down.jpg",
		TEXTUREDIR"GreenSky_front.jpg", TEXTUREDIR"GreenSky_back.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	landNightSkybox = SOIL_load_OGL_cubemap(
		TEXTUREDIR"LightGreen_right.png", TEXTUREDIR"LightGreen_left.png",
		TEXTUREDIR"LightGreen_top.png", TEXTUREDIR"LightGreen_bottom.png",
		TEXTUREDIR"LightGreen_front.png", TEXTUREDIR"LightGreen_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!planetTexture || !planetBumpMap || !asteroidTexture || !asteroidBumpMap || !surfaceTexture 
		|| !surfaceBumpMap || !barkTexture || !barkBumpMap || !waterTex || !spaceSkybox 
		|| !landDaySkybox || !landNightSkybox) return;

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sceneShader = new Shader("PerPixelSceneVertex.glsl", "PerPixelSceneFragment.glsl");
	waterShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");
	fogShader = new Shader("FogVertex.glsl", "FogFragment.glsl");
	waterBlurShader = new Shader("WaterBlurVert.glsl", "WaterBlurFrag.glsl");
	presentShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess() || !waterShader->LoadSuccess()
		|| !shadowShader->LoadSuccess() || !fogShader->LoadSuccess() || !waterBlurShader->LoadSuccess()
		|| !presentShader->LoadSuccess()) return;

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; i++) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &postProcessFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) return;

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetTextureMirrorRepeating(planetTexture, true);
	SetTextureMirrorRepeating(planetBumpMap, true);
	SetTextureRepeating(asteroidTexture, true);
	SetTextureRepeating(asteroidBumpMap, true);
	SetTextureRepeating(surfaceTexture, true);
	SetTextureRepeating(surfaceBumpMap, true);
	SetTextureRepeating(waterTex, true);

	spaceCamera = new Camera(-45.0f, 0.0f, Vector3(0, 30, 175));

	landCamera = new Camera(-45.0f, 0.0f, heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.5f) + Vector3(0.0f, 2400.0f, 0.0f));

	activeCamera = spaceCamera;

	spaceLight = new Light(Vector3(100.0f, 100.0f, 100.0f), Vector4(1, 1, 1, 1), 10000.0f);

	dayLight = new Light(heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.0f), Vector4(1, 1, 1, 1), 40000);

	nightLight = new Light(heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 1.0f), Vector4(0.678f, 0.847f, 0.901f, 1), 20000);

	activeLight = spaceLight;

	spaceRoot = new SceneNode();

	landRoot = new SceneNode();

	SetNodes();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	yMax = 2500.0f;

	spaceCamera->AddNode(Vector3(0, 30, 175));
	spaceCamera->AddNode(Vector3(-1500, 30, -700));
	spaceCamera->AddNode(Vector3(-2100, -1360, -2480));
	spaceCamera->AddNode(Vector3(-690, -2620, -4260));
	spaceCamera->AddNode(Vector3(1790, -690, -3800));
	spaceCamera->AddNode(Vector3(2300, -650, -1400));
	spaceCamera->AddNode(Vector3(0, 30, 175));
	spaceCamera->AddNode(Vector3(0, 30, 175));
	spaceCamera->AddNode(boundingCentre);

	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.5f) + Vector3(0.0f, 2400.0f, 0.0f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.5f, 1.0f, 0.5f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.9f, 1.0f, 0.9f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.9f, 1.0f, 0.1f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.6f, 1.0f, 0.5f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.7f, 0.5f, 0.6f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.7f, 0.5f, 0.6f));
	landCamera->AddNode(heightMap->GetHeightmapSize() * Vector3(0.5f, 1.0f, 0.5f) + Vector3(0, 3000.0f, 0));

	init = true;
}

Renderer::~Renderer(void) {
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(2, bufferColourTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &postProcessFBO);
	delete skyboxShader;
	delete sceneShader;
	delete waterShader;
	delete shadowShader;
	delete fogShader;
	delete waterBlurShader;
	delete presentShader;
	delete quad;
	delete sphere;
	delete asteroid;
	delete tree;
	delete heightMap;
	delete landRoot;
	delete activeCamera;
	if (!planet) delete spaceCamera;
	else delete landCamera;
	delete activeLight;
	if (!planet && day) { delete spaceLight; delete nightLight; }
	else if (!planet && !day) { delete spaceLight; delete dayLight; }
	else { delete dayLight; delete nightLight; }
}

void Renderer::UpdateScene(float dt) {
	if (InTransitionBounds()) {
		SwitchScene();
	}
	activeCamera->UpdateCamera(dt);
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	if (planet) {
		planetCore->SetTransform(planetCore->GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(0, 1, 0)));

		for (vector<SceneNode*>::const_iterator i = planetCore->GetChildIteratorStart(); i != planetCore->GetChildIteratorEnd(); i++) {
			float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX / 50);
			(*i)->SetTransform((*i)->GetTransform() * Matrix4::Rotation(random * dt, Vector3(1, 1, 1)));
		}
		spaceRoot->Update(dt);
	}
	else {
		landRoot->Update(dt);

		lightRoot->SetTransform(lightRoot->GetTransform() * Matrix4::Rotation(5 * dt, Vector3(1, 0, 0)));

		lightRoot->Update(dt);

		Vector4 dayPos = (*(lightRoot->GetChildIteratorStart()))->GetWorldTransform() * Vector4(1, 1, 1, 1);
		Vector4 nightPos = (*(lightRoot->GetChildIteratorStart() + 1))->GetWorldTransform() * Vector4(1, 1, 1, 1);

		dayLight->SetPosition(Vector3(dayPos.x, dayPos.y, dayPos.z));

		nightLight->SetPosition(Vector3(nightPos.x, nightPos.y, nightPos.z));

		DayNightCycle();

		waterRotate += dt * 2.0f;
		waterCycle += dt * 0.25f;
	}
}

void Renderer::RenderScene() {
	BuildNodeLists(planet ? spaceRoot : landRoot);
	SortNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	DrawSkybox();
	if (!planet) DrawWater();
	DrawShadowScene();
	if (!planet) DrawHeightMap();
	DrawNodes();

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	DrawFog();
	if (!planet) DrawWaterBlur();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	PresentScene();

	glEnable(GL_DEPTH_TEST);

	ClearNodeLists();
}

void Renderer::SwitchScene() {
	if (planet) LoadLand();
	else LoadPlanet();
	planet = !planet;
}

void Renderer::LoadPlanet()
{
	activeLight = spaceLight;
	activeCamera = spaceCamera;
	if (InTransitionBounds()) landCamera->SetPosition(landCamera->GetNextNode());
}

void Renderer::LoadLand()
{
	activeLight = (day ? dayLight : nightLight);
	activeCamera = landCamera;
	if (InTransitionBounds()) spaceCamera->SetPosition(spaceCamera->GetNextNode());
}

bool Renderer::InTransitionBounds()
{
	if (!planet) return(landCamera->GetPosition().y > yMax);

	Vector3 pos = spaceCamera->GetPosition();
	int x = pow((pos.x - boundingCentre.x), 2);
	int y = pow((pos.y - boundingCentre.y), 2);
	int z = pow((pos.z - boundingCentre.z), 2);

	if (x + y + z == boundingRadius * boundingRadius || x + y + z < boundingRadius * boundingRadius) return true;

	else return false;
}

void Renderer::DayNightCycle()
{
	if (day) {
		if (dayLight->GetPosition().y < 0) { activeLight = nightLight; day = false; }
	}
	else if (!day) {
		if (nightLight->GetPosition().y < 0) { activeLight = dayLight; day = true; }
	}
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	if (planet) glBindTexture(GL_TEXTURE_CUBE_MAP, spaceSkybox);
	else if (day) glBindTexture(GL_TEXTURE_CUBE_MAP, landDaySkybox);
	else glBindTexture(GL_TEXTURE_CUBE_MAP, landNightSkybox);

	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightMap()
{
	BindShader(sceneShader);
	SetShaderLight(*activeLight);
	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surfaceTexture);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, surfaceBumpMap);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();
	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(waterShader);

	glUniform3fv(glGetUniformLocation(waterShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	if (planet) glBindTexture(GL_TEXTURE_CUBE_MAP, spaceSkybox);
	else if (day) glBindTexture(GL_TEXTURE_CUBE_MAP, landDaySkybox);
	else glBindTexture(GL_TEXTURE_CUBE_MAP, landNightSkybox);

	Vector3 hSize = heightMap->GetHeightmapSize();

	modelMatrix = Matrix4::Translation(hSize * 0.5f - Vector3(0, 35.0f, 0)) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
	textureMatrix.ToIdentity();
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(activeLight->GetPosition(), (planet ? boundingCentre : heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.5f)));
	projMatrix = Matrix4::Perspective(1, 15000, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	DrawShadowNodes();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
}

void Renderer::DrawFog() {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);

	BindShader(fogShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	float distance = 0;
	float visibility = 0;
	if (planet) {
		distance = sqrt(pow(activeCamera->GetPosition().x - boundingCentre.x, 2) +
			pow(activeCamera->GetPosition().y - boundingCentre.y, 2) + pow(activeCamera->GetPosition().z - boundingCentre.z, 2));
		visibility = distance > 2200.0f ? 1 : (distance - 1500) / 700;
	}
	else {
		distance = yMax - activeCamera->GetPosition().y;
		visibility = distance > 500.0f ? 1 : distance / 500;
	}
	glUniform1f(glGetUniformLocation(fogShader->GetProgram(), "visibility"), visibility);
	glUniform3fv(glGetUniformLocation(fogShader->GetProgram(), "fogColour"), 1, (float*)&Vector3(0.286f, 0.407f, 0));
	quad->Draw();
}

void Renderer::DrawWaterBlur()
{
	if (activeCamera->GetPosition().y < 92.5f && activeCamera->GetPosition().y > heightMap->GetHeightAt(activeCamera->GetPosition().x, activeCamera->GetPosition().z)) {
		BindShader(waterBlurShader);
		UpdateShaderMatrices();
		glUniform3fv(glGetUniformLocation(waterBlurShader->GetProgram(), "waterColour"), 1, (float*)&Vector3(0.509f, 1.0f, 0.518f));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(waterBlurShader->GetProgram(), "sceneTex"), 0);
		for (int i = 0; i < 4; i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
			glUniform1i(glGetUniformLocation(waterBlurShader->GetProgram(), "isVertical"), 0);

			glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
			quad->Draw();

			glUniform1i(glGetUniformLocation(waterBlurShader->GetProgram(), "isVertical"), 1);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
			glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
			quad->Draw();
		}
	}
}

void Renderer::PresentScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(presentShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(presentShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
	quad->Draw();
}

void Renderer::SetNodes() {
	SceneNode* p = new SceneNode();
	p->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	p->SetTransform(Matrix4::Translation(Vector3(0, -2000.0f, -2000.0f)));
	p->SetModelScale(Vector3(1000.0f, 1000.0f, 1000.0f));
	p->SetBoundingRadius(1000.0f);
	p->SetMesh(sphere);
	p->SetTexture(planetTexture);
	p->SetBumpMap(planetBumpMap);
	spaceRoot->AddChild(p);

	SceneNode* core = new SceneNode();
	p->AddChild(core);
	planetCore = core;

	double pi = 2 * acos(0.0);
	for (int i = 0; i < 8; i++) {
		SceneNode* a = new SceneNode();
		a->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		float angle = i * 2 * pi / 8;
		a->SetTransform(Matrix4::Translation(Vector3(1300.0f * cos(angle), 0, 1300.0f * sin(angle))));
		a->SetModelScale(Vector3(200.0f, 200.0f, 200.0f));
		a->SetBoundingRadius(200.0f);
		a->SetMesh(asteroid);
		a->SetTexture(asteroidTexture);
		a->SetBumpMap(asteroidBumpMap);
		core->AddChild(a);
	}
	boundingCentre = Vector3(0, -2000.0f, -2000.0f);
	boundingRadius = 1500.0f;

	SceneNode* l = new SceneNode();
	l->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.5f)));
	lightRoot = l;
	SceneNode* d = new SceneNode();
	d->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.0f, 0.0f, -0.5f)));
	lightRoot->AddChild(d);
	SceneNode* n = new SceneNode();
	n->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.0f, 0.0f, 0.5f)));
	lightRoot->AddChild(n);
	for (int i = 0; i < 10; i++) {
		SceneNode* t = new SceneNode();
		t->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		int randomX = rand() / (RAND_MAX / 8176);
		int randomZ = rand() / (RAND_MAX / 8176);
		while (heightMap->GetHeightAt(randomX, randomZ) < 92.5f) {
			randomX = rand() / (RAND_MAX / 8176);
			randomZ = rand() / (RAND_MAX / 8176);
		}
		t->SetTransform(Matrix4::Translation(Vector3(randomX, heightMap->GetHeightAt(randomX, randomZ), randomZ)));
		t->SetModelScale(Vector3(20.0f, 20.0f, 20.0f));
		t->SetBoundingRadius(450.0f);
		t->SetMesh(tree);
		t->SetTexture(barkTexture);
		t->SetBumpMap(barkBumpMap);
		landRoot->AddChild(t);
	}
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - activeCamera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) transparentNodeList.push_back(from);
		else nodeList.push_back(from);
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) BuildNodeLists(*i);
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::DrawNodes() {
	BindShader(sceneShader);
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	UpdateShaderMatrices();
	SetShaderLight(*activeLight);
	for (const auto& i : nodeList) DrawNode(i);
	for (const auto& i : transparentNodeList) DrawNode(i);
	ClearNodeLists();
}

void Renderer::DrawShadowNodes() {
	for (const auto& i : nodeList) { modelMatrix = i->GetWorldTransform() * Matrix4::Scale(i->GetModelScale()); UpdateShaderMatrices(); i->Draw(*this); }
	for (const auto& i : transparentNodeList) { modelMatrix = i->GetWorldTransform() * Matrix4::Scale(i->GetModelScale()); UpdateShaderMatrices(); i->Draw(*this); }
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

		glUniformMatrix4fv(glGetUniformLocation(sceneShader->GetProgram(), "modelMatrix"), 1, false, model.values);

		glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		GLuint texture = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		GLuint bumpMap = n->GetBumpMap();
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bumpMap);

		n->Draw(*this);
	}
}