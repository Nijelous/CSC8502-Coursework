#pragma once
#include "../NCLGL/OGLRenderer.h"
#include "../nclgl/Frustum.h"

class Camera;
class SceneNode;
class Light;
class HeightMap;
class MeshAnimation;
class MeshMaterial;

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
	void DrawAnimation();
	void DrawShadowAnimation();

	void DrawShadowScene();

	void DrawWaterBlur();
	void DrawFog();
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
	Mesh* roleT;
	HeightMap* heightMap;
	MeshAnimation* anim;
	MeshMaterial* animMaterial;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* animShader;
	Shader* waterShader;
	Shader* shadowShader;
	Shader* shadowAnimShader;
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
	GLuint leavesTexture;
	GLuint leavesBumpMap;
	GLuint waterTex;
	GLuint spaceSkybox;
	GLuint landDaySkybox;
	GLuint landNightSkybox;
	vector<GLuint> matTextures;

	Vector3 boundingCentre;
	float boundingRadius;

	bool planet = true;
	bool day = true;

	float yMax;

	float waterRotate;
	float waterCycle;

	int currentFrame;
	float frameTime;
	Matrix4 animModel;
	Vector3 animPos;
	int direction;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
