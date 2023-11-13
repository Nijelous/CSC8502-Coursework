#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"

class Camera;
class SceneNode;
class Light;
class HeightMap;

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);
	 void UpdateScene(float dt)	override;
	 void RenderScene()	override;

protected:
	void SwitchScene();

	void LoadPlanet();
	void LoadLand();

	bool InTransitionBounds();

	void DayNightCycle();

	void DrawSkybox();
	void DrawHeightMap();
	void DrawWater();

	void DrawShadowScene();

	void DrawFog();
	void DrawWaterBlur();
	void PresentScene();

	void SetNodes();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawShadowNodes();
	void DrawNode(SceneNode* n);

	Mesh* quad;
	Mesh* sphere;
	Mesh* asteroid;
	Mesh* tree;
	HeightMap* heightMap;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* waterShader;
	Shader* shadowShader;
	Shader* fogShader;
	Shader* waterBlurShader;
	Shader* presentShader;

	SceneNode* spaceRoot;
	SceneNode* planetCore;
	SceneNode* landRoot;
	SceneNode* lightRoot;
	Camera* activeCamera;
	Camera* spaceCamera;
	Camera* landCamera;
	Light* activeLight;
	Light* spaceLight;
	Light* dayLight;
	Light* nightLight;

	GLuint shadowFBO;
	GLuint bufferFBO;
	GLuint postProcessFBO;

	GLuint shadowTex;
	GLuint bufferDepthTex;
	GLuint bufferColourTex[2];

	GLuint planetTexture;
	GLuint planetBumpMap;
	GLuint asteroidTexture;
	GLuint asteroidBumpMap;
	GLuint surfaceTexture;
	GLuint surfaceBumpMap;
	GLuint barkTexture;
	GLuint barkBumpMap;
	GLuint waterTex;
	GLuint spaceSkybox;
	GLuint landDaySkybox;
	GLuint landNightSkybox;

	Vector3 boundingCentre;
	float boundingRadius;

	bool planet = true;
	bool day = true;

	float yMax;

	float waterRotate;
	float waterCycle;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
