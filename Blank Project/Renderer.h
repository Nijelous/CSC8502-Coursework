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
	 void UpdateScene(float dt)	override;
	 void SwitchToScene() override;
	 bool InTransitionBounds();
	 void SwitchFromScene();
protected:
	void DeleteShaders();

	void DrawPostProcess();
	void PresentScene();

	bool LoadShaders();

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

	bool active = true;

	Mesh* quad;
	Mesh* sphere;
	Mesh* asteroid;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* shadowShader;
	Shader* postProcessShader;
	Shader* presentShader;

	SceneNode* root;
	SceneNode* planetCore;
	Camera* camera;
	Light* light;

	GLuint shadowTex;
	GLuint shadowFBO;
	GLuint bufferFBO;
	GLuint postProcessFBO;

	GLuint bufferDepthTex;
	GLuint bufferColourTex[2];

	GLuint planetTexture;
	GLuint planetBumpMap;
	GLuint asteroidTexture;
	GLuint asteroidBumpMap;
	GLuint cubeMap;

	Vector3 boundingCentre;
	float boundingRadius;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
