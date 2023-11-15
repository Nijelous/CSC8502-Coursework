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
	 void ToggleThirdPersonCamera() { if (!planet) thirdPerson = !thirdPerson; }
	 void ToogleFog() { if (!planet) hasFog = !hasFog; }
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

	void DrawShadowScene(Camera* camera);

	void DrawWaterBlur(Camera* camera);
	void DrawFogScreenSpace();
	void DrawFogSceneSpace();
	void PresentScene();

	void SetNodes();
	void BuildNodeListsFrame(SceneNode* from);
	void BuildNodeListsThirdPerson(SceneNode* from);
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
	Shader* fogSceneShader;
	Shader* waterBlurShader;
	Shader* presentShader;

	SceneNode* spaceRoot;
	SceneNode* planetCore;
	SceneNode* landRoot;
	SceneNode* lightRoot;
	Camera* activeCamera;
	Camera* spaceCamera;
	Camera* landCamera;
	Camera* thirdPersonCamera;
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
	bool thirdPerson = false;
	bool hasFog = true;

	float yMax;

	float waterRotate;
	float waterCycle;

	int currentFrame;
	float frameTime;
	Matrix4 animModel;
	Vector3 animPos;
	int direction;
	float density;
	float gradient;
	Vector3 fogColour;
	float skyMixing;

	Frustum frameFrustum;
	Frustum thirdPersonFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;
};
