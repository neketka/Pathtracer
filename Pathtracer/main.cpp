#include <iostream>
#include <SDL2/SDL.h>
#include <Windows.h>

import window;
import extengine;
import pathtracingsystem;
import movementsystem;
import demosystem;
import assetsystem;
import gpuscenesystem;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	if (SDL_Init(SDL_VIDEO_OPENGL) != 0) {
		std::cerr << "Failed to init SDL with OpenGL!" << std::endl;
		return -1;
	}

	ExtEngineBase<MovementSystem, PathtracingSystem, AssetSystem, GpuSceneSystem> engine;
	Window<ExtEngine> w("Pathtracer", 800, 600, engine);

	engine.getSystem<AssetSystem>().loadFrom("./assets");
	w.start();

	SDL_Quit();

	return 0;
}
