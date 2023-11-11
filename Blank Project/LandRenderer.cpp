#include "LandRenderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Camera.h"
#include <algorithm>

#define SHADOWSIZE 2048

LandRenderer::LandRenderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	init = LoadShaders();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	yMax = 2500.0f;

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));

	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), 10000);

	root = new SceneNode();

	//SetNodes();

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	waterRotate = 0.0f;
	waterCycle = 0.0f;

	camera->AddNode(heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
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
		glDeleteTextures(1, &shadowTex);
		glDeleteFramebuffers(1, &shadowFBO);
		delete skyboxShader;
		delete sceneShader;
		delete landShader;
		delete waterShader;
	}
	delete heightMap;
	delete quad;
	delete root;
	delete camera;
	delete light;
}

void LandRenderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);

	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
}

void LandRenderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();

	//std::cout << camera->GetPosition() << "\n";

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSkybox();
	DrawHeightMap();
	DrawWater();
	DrawNodes();
	ClearNodeLists();
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
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	delete skyboxShader;
	delete sceneShader;
	delete landShader;
	delete waterShader;
	active = false;
}

bool LandRenderer::LoadShaders()
{
	heightMap = new HeightMap(TEXTUREDIR"swampHeightmap.png");

	surfaceTexture = SOIL_load_OGL_texture(TEXTUREDIR"Swampland.JPG", SOIL_LOAD_AUTO, 1, SOIL_FLAG_MIPMAPS);
	surfaceBumpMap = SOIL_load_OGL_texture(TEXTUREDIR"SwamplandDOT3.JPG", SOIL_LOAD_AUTO, 2, SOIL_FLAG_MIPMAPS);

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

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess() || !landShader->LoadSuccess() || !waterShader->LoadSuccess()) return false;

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
}

void LandRenderer::SetNodes() {

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