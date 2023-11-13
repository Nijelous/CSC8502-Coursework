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
	void DayNightCycle();

	void DeleteShaders();

	bool LoadShaders();

	void DrawFog();
	void DrawWaterBlur();
	void PresentScene();

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
	Shader* fogShader;
	Shader* waterBlurShader;
	Shader* presentShader;

	SceneNode* root;
	SceneNode* lightRoot;
	Camera* camera;
	Light* light;
	Light* dayLight;
	Light* nightLight;

	GLuint shadowFBO;
	GLuint bufferFBO;
	GLuint postProcessFBO;

	GLuint shadowTex;
	GLuint bufferDepthTex;
	GLuint bufferColourTex[3];

	GLuint surfaceTexture;
	GLuint surfaceBumpMap;
	GLuint sunTexture;
	GLuint sunBumpMap;
	GLuint cubeMap;
	GLuint waterTex;

	bool day = true;

	float yMax;

	float waterRotate;
	float waterCycle;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

};

