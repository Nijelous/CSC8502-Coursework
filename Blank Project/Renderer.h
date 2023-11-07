#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"

class Camera;
class SceneNode;
class Light;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;
protected:
	void DrawShadowScene();
	void DrawMainScene();

	void DrawSkybox();

	void SetNodes();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawShadowNodes();
	void DrawNode(SceneNode* n);

	Mesh* quad;
	Mesh* sphere;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* shadowShader;

	SceneNode* root;
	SceneNode* planetCore;
	Camera* camera;
	Light* light;

	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint planetTexture;
	GLuint planetBumpMap;
	GLuint asteroidTexture;
	GLuint asteroidBumpMap;
	GLuint cubeMap;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
