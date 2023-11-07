#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Light.h"
#include <algorithm>

#define SHADOWSIZE 2048

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	planetTexture = SOIL_load_OGL_texture(TEXTUREDIR"planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planetBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"planetDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	asteroidTexture = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	asteroidBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"SkyBlue_right.png", TEXTUREDIR"SkyBlue_left.png",
		TEXTUREDIR"SkyBlue_top.png", TEXTUREDIR"SkyBlue_bottom.png",
		TEXTUREDIR"SkyBlue_front.png", TEXTUREDIR"SkyBlue_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!cubeMap || !planetTexture || !planetBumpMap || !asteroidTexture || !asteroidBumpMap) return;

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sceneShader = new Shader("PerPixelSceneVertex.glsl", "PerPixelSceneFragment.glsl");
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess() || !shadowShader->LoadSuccess()) return;

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

	SetTextureMirrorRepeating(planetTexture, true);
	SetTextureMirrorRepeating(planetBumpMap, true);

	camera = new Camera(-45.0f, 0.0f, Vector3(0, 30, 175));

	light = new Light(Vector3(100.0f, 100.0f, 100.0f), Vector4(1, 1, 1, 1), 10000.0f);

	root = new SceneNode();

	SetNodes();

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	init = true;
}

Renderer::~Renderer(void)	{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	delete quad;
	delete skyboxShader;
	delete sceneShader;
	delete root;
	delete camera;
	delete light;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	planetCore->SetTransform(planetCore->GetTransform() * Matrix4::Rotation(-30.0f * dt, Vector3(0, 1, 0)));

	root->Update(dt);
}

void Renderer::RenderScene()	{
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSkybox();
	DrawShadowScene();
	DrawNodes();
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	DrawShadowNodes();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
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
		a->SetModelScale(Vector3(50.0f, 50.0f, 50.0f));
		a->SetBoundingRadius(50.0f);
		a->SetMesh(sphere);
		a->SetTexture(asteroidTexture);
		a->SetBumpMap(asteroidBumpMap);
		core->AddChild(a);
	}
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