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

protected:
	bool LoadShaders();

	void DrawSkybox();
	void DrawHeightMap();

	void SetNodes();
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawShadowNodes();
	void DrawNode(SceneNode* n);

	Mesh* quad;
	HeightMap* heightMap;

	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* landShader;

	SceneNode* root;
	Camera* camera;
	Light* light;

	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint surfaceTexture;
	GLuint surfaceBumpMap;
	GLuint cubeMap;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

};

