#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include <algorithm>

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");

	planetTexture = SOIL_load_OGL_texture(TEXTUREDIR"planet.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"SkyBlue_right.png", TEXTUREDIR"SkyBlue_left.png",
		TEXTUREDIR"SkyBlue_top.png", TEXTUREDIR"SkyBlue_bottom.png",
		TEXTUREDIR"SkyBlue_front.png", TEXTUREDIR"SkyBlue_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!cubeMap || !planetTexture) return;

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	if (!skyboxShader->LoadSuccess() || !sceneShader->LoadSuccess()) return;

	camera = new Camera(-45.0f, 0.0f, Vector3(0, 30, 175));

	root = new SceneNode();

	SceneNode* s = new SceneNode();
	s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	s->SetTransform(Matrix4::Translation(Vector3(0, -500.0f, -500.0f)));
	s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
	s->SetBoundingRadius(100.0f);
	s->SetMesh(sphere);
	s->SetTexture(planetTexture);
	root->AddChild(s);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	init = true;
}

Renderer::~Renderer(void)	{
	delete quad;
	delete skyboxShader;
	delete sceneShader;
	delete root;
	delete camera;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);
}

void Renderer::RenderScene()	{
	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSkybox();
	DrawNodes();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
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

void Renderer::DrawNodes() {
	BindShader(sceneShader);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);

	UpdateShaderMatrices();
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

		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "useTexture"), texture);

		n->Draw(*this);
	}
}