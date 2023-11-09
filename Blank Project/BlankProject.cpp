#include "../NCLGL/window.h"
#include "Renderer.h"
#include "LandRenderer.h"

int main()	{
	Window w("Coursework: Dagobah (ish)", 1280, 720, false);

	if(!w.HasInitialised()) return -1;
	
	LandRenderer landRenderer(w);
	if (!landRenderer.HasInitialised()) return -1;

	Renderer planetRenderer(w);
	if (!planetRenderer.HasInitialised()) return -1;

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	bool planet = true;

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		if (planet) {
			planetRenderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
			planetRenderer.RenderScene();
			planetRenderer.SwapBuffers();
		}
		else {
			landRenderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
			landRenderer.RenderScene();
			landRenderer.SwapBuffers();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_TAB)) {
			planet = !planet;
			if (!planet) landRenderer.SwitchToScene();
			else planetRenderer.SwitchToScene();
			if (!landRenderer.HasInitialised() || !planetRenderer.HasInitialised()) return -1;
		}
	}
	return 0;
}