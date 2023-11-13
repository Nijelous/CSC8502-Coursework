#include "LandRenderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Matrix3.h"
#include <algorithm>

#define SHADOWSIZE 2048

LandRenderer::LandRenderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	init = LoadShaders();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	yMax = 2500.0f;

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 0.0f, 0.5f) + Vector3(0.0f, 2400.0f, 0.0f));

	dayLight = new Light(heightmapSize * Vector3(0.5f, 0.0f, 0.5f), Vector4(1, 1, 1, 1), 40000);
	nightLight = new Light(heightmapSize * Vector3(0.5f, 0.0f, 1.0f), Vector4(0.678f, 0.847f, 0.901f, 1), 20000);

	light = dayLight;

	root = new SceneNode();

	SetNodes();

	Vector4 lightPos = (lightRoot->GetTransform() * Vector4(1, 1, 1, 1));

	std::cout << (Vector3(lightPos.x-1, lightPos.y-1, lightPos.z-1)) << "\n";

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	camera->AddNode(heightmapSize * Vector3(0.5f, 0.0f, 0.5f) + Vector3(0.0f, 2400.0f, 0.0f));
	camera->AddNode(heightmapSize * Vector3(0.5f, 1.0f, 0.5f));
	camera->AddNode(heightmapSize * Vector3(0.9f, 1.0f, 0.9f));
	camera->AddNode(heightmapSize * Vector3(0.9f, 1.0f, 0.1f));
	camera->AddNode(heightmapSize * Vector3(0.6f, 1.0f, 0.5f));
	camera->AddNode(heightmapSize * Vector3(0.7f, 0.5f, 0.6f));
	camera->AddNode(heightmapSize * Vector3(0.7f, 0.5f, 0.6f));
	camera->AddNode(heightmapSize * Vector3(0.5f, 1.0f, 0.5f) + Vector3(0, 3000.0f, 0));
}

LandRenderer::~LandRenderer() {
	if (active) {
		DeleteShaders();
	}
	delete quad;
	delete root;
	delete lightRoot;
	delete camera;
	delete light;
	if(!day) delete dayLight;
	else delete nightLight;
}

void LandRenderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);

	lightRoot->SetTransform(lightRoot->GetTransform() * Matrix4::Rotation(5*dt, Vector3(1, 0, 0)));

	lightRoot->Update(dt);

	Vector4 dayPos = (*(lightRoot->GetChildIteratorStart()))->GetWorldTransform() * Vector4(1, 1, 1, 1);
	Vector4 nightPos = (*(lightRoot->GetChildIteratorStart() + 1))->GetWorldTransform() * Vector4(1, 1, 1, 1);

	dayLight->SetPosition(Vector3(dayPos.x, dayPos.y, dayPos.z));

	nightLight->SetPosition(Vector3(nightPos.x, nightPos.y, nightPos.z));

	DayNightCycle();

	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
}

void LandRenderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	DrawSkybox();
	DrawHeightMap();
	DrawWater();
	DrawNodes();

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	DrawWaterBlur();
	DrawFog();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	PresentScene();

	glEnable(GL_DEPTH_TEST);

	ClearNodeLists();
}

void LandRenderer::DrawFog() {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	BindShader(fogShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	float distance = yMax - camera->GetPosition().y;
	glUniform1f(glGetUniformLocation(fogShader->GetProgram(), "visibility"), distance > 500.0f ? 1 : distance / 500);
	glUniform3fv(glGetUniformLocation(fogShader->GetProgram(), "fogColour"), 1, (float*)&Vector3(0.286f, 0.407f, 0));
	quad->Draw();
}

void LandRenderer::DrawWaterBlur() {
	if (camera->GetPosition().y < 92.5f && camera->GetPosition().y > heightMap->GetHeightAt(camera->GetPosition().x, camera->GetPosition().z)) {
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

void LandRenderer::PresentScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(presentShader);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(presentShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
	quad->Draw();
}


void LandRenderer::SwitchToScene() {
	init = LoadShaders();
	active = true;
}

bool LandRenderer::InTransitionBounds()
{
	return(camera->GetPosition().y > yMax);
}

void LandRenderer::SwitchFromScene()
{
	if (InTransitionBounds()) {
		camera->SetPosition(camera->GetNextNode());
	}
	DeleteShaders();
	active = false;
}

void LandRenderer::DayNightCycle()
{
	if (day) {
		if (dayLight->GetPosition().y < 0) { light = nightLight; day = false; }
	}
	else if (!day) {
		if (nightLight->GetPosition().y < 0) { light = dayLight; day = true; }
	}
}

void LandRenderer::DeleteShaders() {
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(3, bufferColourTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &postProcessFBO);
	delete skyboxShader;
	delete sceneShader;
	delete landShader;
	delete waterShader;
	delete fogShader;
	delete waterBlurShader;
	delete presentShader;
	delete heightMap;
}

bool LandRenderer::LoadShaders()
{
	heightMap = new HeightMap(TEXTUREDIR"swampHeightmap.png");

	surfaceTexture = SOIL_load_OGL_texture(TEXTUREDIR"Swampland.JPG", SOIL_LOAD_AUTO, 1, SOIL_FLAG_MIPMAPS);
	surfaceBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"SwamplandDOT3.JPG", SOIL_LOAD_AUTO, 2, SOIL_FLAG_MIPMAPS);

	sunTexture = SOIL_load_OGL_texture(TEXTUREDIR"Sun.JPG", SOIL_LOAD_AUTO, 3, SOIL_FLAG_MIPMAPS);
	sunBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"SunDOT3.JPG", SOIL_LOAD_AUTO, 4, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"GreenSky_right.jpg", TEXTUREDIR"GreenSky_left.jpg",
		TEXTUREDIR"GreenSky_up.jpg", TEXTUREDIR"GreenSky_down.jpg",
		TEXTUREDIR"GreenSky_front.jpg", TEXTUREDIR"GreenSky_back.jpg", SOIL_LOAD_RGB, 5, 0);

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"swampWater.TGA", SOIL_LOAD_AUTO, 6, SOIL_FLAG_MIPMAPS);

	if (!cubeMap || !surfaceTexture || !surfaceBumpMap || !waterTex) return false;

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sceneShader = new Shader("PerPixelSceneVertex.glsl", "PerPixelSceneFragment.glsl");
	landShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	waterShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");
	fogShader = new Shader("FogVertex.glsl", "FogFragment.glsl");
	waterBlurShader = new Shader("WaterBlurVert.glsl", "WaterBlurFrag.glsl");
	presentShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess() || !landShader->LoadSuccess() || !waterShader->LoadSuccess()
		|| !fogShader->LoadSuccess() || !waterBlurShader->LoadSuccess() || !presentShader->LoadSuccess()) return false;

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

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetTextureRepeating(surfaceTexture, true);
	SetTextureRepeating(surfaceBumpMap, true);
	SetTextureRepeating(waterTex, true);

	return true;
}

void LandRenderer::DrawSkybox() {
 	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

 	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}

void LandRenderer::DrawHeightMap() {
	BindShader(landShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(landShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(landShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surfaceTexture);

	glUniform1i(glGetUniformLocation(landShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, surfaceBumpMap);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();
	heightMap->Draw();
}

void LandRenderer::DrawWater() {
	BindShader(waterShader);

	glUniform3fv(glGetUniformLocation(waterShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();

	modelMatrix = Matrix4::Translation(hSize * 0.5f - Vector3(0, 35.0f, 0)) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * 
		Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
	textureMatrix.ToIdentity();
}

void LandRenderer::SetNodes() {
	SceneNode* p = new SceneNode();
	p->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.5f, 0.0f, 0.5f)));
	lightRoot = p;
	SceneNode* d = new SceneNode();
	d->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.0f, 0.0f, -0.5f)));
	lightRoot->AddChild(d);
	SceneNode* n = new SceneNode();
	n->SetTransform(Matrix4::Translation(heightMap->GetHeightmapSize() * Vector3(0.0f, 0.0f, 0.5f)));
	lightRoot->AddChild(n);
}

void LandRenderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) transparentNodeList.push_back(from);
		else nodeList.push_back(from);
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) BuildNodeLists(*i);
}

void LandRenderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void LandRenderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void LandRenderer::DrawShadowNodes() {
	for (const auto& i : nodeList) i->Draw(*this);
	for (const auto& i : transparentNodeList) i->Draw(*this);
}

void LandRenderer::DrawNodes() {
	BindShader(sceneShader);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	UpdateShaderMatrices();
	SetShaderLight(*light);
	for (const auto& i : nodeList) DrawNode(i);
	for (const auto& i : transparentNodeList) DrawNode(i);
	ClearNodeLists();
}

void LandRenderer::DrawNode(SceneNode* n) {
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