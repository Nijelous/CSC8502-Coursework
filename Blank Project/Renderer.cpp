#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Light.h"
#include <algorithm>

#define SHADOWSIZE 2048

#define POST_PASSES 10

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	asteroid = Mesh::LoadFromMeshFile("Rock.msh");

	init = LoadShaders();

	camera = new Camera(-45.0f, 0.0f, Vector3(0, 30, 175));

	light = new Light(Vector3(100.0f, 100.0f, 100.0f), Vector4(1, 1, 1, 1), 10000.0f);

	root = new SceneNode();

	SetNodes();

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	camera->AddNode(Vector3(0, 30, 175));
	camera->AddNode(Vector3(-1500, 30, -700));
	camera->AddNode(Vector3(-2100, -1360, -2480));
	camera->AddNode(Vector3(-690, -2620, -4260));
	camera->AddNode(Vector3(1790, -690, -3800));
	camera->AddNode(Vector3(2300, -650, -1400));
	camera->AddNode(Vector3(0, 30, 175));
	camera->AddNode(Vector3(0, 30, 175));
	camera->AddNode(boundingCentre);
}

Renderer::~Renderer(void) {
	if (active) {
		glDeleteTextures(1, &shadowTex);
		glDeleteFramebuffers(1, &shadowFBO);
		glDeleteTextures(1, &bufferDepthTex);
		glDeleteTextures(2, bufferColourTex);
		glDeleteFramebuffers(1, &bufferFBO);
		glDeleteFramebuffers(1, &postProcessFBO);
		delete skyboxShader;
		delete sceneShader;
		delete shadowShader;
		delete postProcessShader;
		delete presentShader;
	}
	delete quad;
	delete root;
	delete camera;
	delete light;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	planetCore->SetTransform(planetCore->GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(0, 1, 0)));

	for (vector<SceneNode*>::const_iterator i = planetCore->GetChildIteratorStart(); i != planetCore->GetChildIteratorEnd(); i++) {
		float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX/50);
		(*i)->SetTransform((*i)->GetTransform() * Matrix4::Rotation(random * dt, Vector3(1, 1, 1)));
	}

	root->Update(dt);
}

void Renderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	DrawSkybox();
	DrawShadowScene();
	DrawNodes();

	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);

	DrawPostProcess();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	PresentScene();
}

void Renderer::DrawPostProcess() {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(postProcessShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(postProcessShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	float distance = sqrt(pow(camera->GetPosition().x - boundingCentre.x, 2) +
		pow(camera->GetPosition().y - boundingCentre.y, 2) + pow(camera->GetPosition().z - boundingCentre.z, 2));
	glUniform1f(glGetUniformLocation(postProcessShader->GetProgram(), "visibility"), distance > 2200.0f ? 1 : (distance - 1500) / 700);
	glUniform3fv(glGetUniformLocation(postProcessShader->GetProgram(), "fogColour"), 1, (float*)&Vector3(0.286f, 0.407f, 0));
	quad->Draw();
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(presentShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(presentShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
	quad->Draw();
}

void Renderer::SwitchToScene() {
	init = LoadShaders();
	active = true;
}

bool Renderer::InTransitionBounds()
{
	Vector3 pos = camera->GetPosition();
	int x = pow((pos.x - boundingCentre.x), 2);
	int y = pow((pos.y - boundingCentre.y), 2);
	int z = pow((pos.z - boundingCentre.z), 2);

	if (x + y + z == boundingRadius * boundingRadius || x + y + z < boundingRadius * boundingRadius) return true;

	else return false;
}

void Renderer::SwitchFromScene()
{
	if (InTransitionBounds()) {
		camera->SetPosition(camera->GetNextNode());
	}
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(2, bufferColourTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &postProcessFBO);
	delete skyboxShader;
	delete sceneShader;
	delete shadowShader;
	delete postProcessShader;
	delete presentShader;
	active = false;
}

bool Renderer::LoadShaders()
{
	planetTexture = SOIL_load_OGL_texture(TEXTUREDIR"planet.JPG", SOIL_LOAD_AUTO, 1, SOIL_FLAG_MIPMAPS);
	planetBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"planetDOT3.JPG", SOIL_LOAD_AUTO, 2, SOIL_FLAG_MIPMAPS);

	asteroidTexture = SOIL_load_OGL_texture(TEXTUREDIR"Rock.png", SOIL_LOAD_AUTO, 3, SOIL_FLAG_MIPMAPS);
	asteroidBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"RockDOT3.JPG", SOIL_LOAD_AUTO, 4, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"SkyBlue_right.png", TEXTUREDIR"SkyBlue_left.png",
		TEXTUREDIR"SkyBlue_top.png", TEXTUREDIR"SkyBlue_bottom.png",
		TEXTUREDIR"SkyBlue_front.png", TEXTUREDIR"SkyBlue_back.png", SOIL_LOAD_RGB, 5, 0);

	if (!cubeMap || !planetTexture || !planetBumpMap || !asteroidTexture || !asteroidBumpMap) return false;

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sceneShader = new Shader("PerPixelSceneVertex.glsl", "PerPixelSceneFragment.glsl");
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");
	postProcessShader = new Shader("FogVertex.glsl", "FogFragment.glsl");
	presentShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess() || !shadowShader->LoadSuccess()) return false;

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

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetTextureMirrorRepeating(planetTexture, true);
	SetTextureMirrorRepeating(planetBumpMap, true);
	SetTextureRepeating(asteroidTexture, true);
	SetTextureRepeating(asteroidBumpMap, true);

	return true;
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	DrawShadowNodes();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
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
	root->AddChild(p);

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
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
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

void Renderer::DrawShadowNodes() {
	for (const auto& i : nodeList) i->Draw(*this);
	for (const auto& i : transparentNodeList) i->Draw(*this);
}

void Renderer::DrawNodes() {
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