#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"

class Camera;
class SceneNode;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
protected:
	void DrawSkybox();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);

	Mesh* quad;
	Mesh* sphere;

	Shader* skyboxShader;
	Shader* sceneShader;

	SceneNode* root;
	Camera* camera;

	GLuint planetTexture;
	GLuint cubeMap;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
