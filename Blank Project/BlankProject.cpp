#include "../NCLGL/window.h"
#include "Renderer.h"

int main()	{
	Window w("Coursework: Dagobah (ish)", 1280, 760, false);

	if(!w.HasInitialised()) return -1;

	Renderer renderer(w);
	if (!renderer.HasInitialised()) return -1;

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	w.GetTimer()->StartTimer();

	while(w.UpdateWindow()  && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		renderer.RenderScene();
		renderer.SwapBuffers();

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T)) {
			renderer.ToggleThirdPersonCamera();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_F)) {
			renderer.ToogleFog();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P)) {
			renderer.PrintPosition();
		}
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R)) {
			renderer.ToggleRain();
		}
	}
	return 0;
}