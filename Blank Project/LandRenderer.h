#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Frustum.h"

class Camera;
class SceneNode;
class Light;
class HeightMap;

class LandRenderer : public OGLRenderer
{
public:
	LandRenderer(Window& parent);
	~LandRenderer();
	void RenderScene() override;
	void UpdateScene(float dt) override;
	void SwitchToScene() override;
	bool InTransitionBounds();
	void SwitchFromScene();

protected:
	bool LoadShaders();

	void DrawSkybox();
	void DrawHeightMap();
	void DrawWater();

	void SetNodes();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawShadowNodes();
	void DrawNode(SceneNode* n);

	bool active = true;

	Mesh* quad;
	HeightMap* heightMap;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* landShader;
	Shader* waterShader;

	SceneNode* root;
	Camera* camera;
	Light* light;

	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint surfaceTexture;
	GLuint surfaceBumpMap;
	GLuint cubeMap;
	GLuint waterTex;

	float yMax;

	float waterRotate;
	float waterCycle;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

};

