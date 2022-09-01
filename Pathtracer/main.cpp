#include <iostream>
#include <SDL2/SDL.h>
#include <Windows.h>

import window;
import engine;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	if (SDL_Init(SDL_VIDEO_OPENGL) != 0) {
		std::cerr << "Failed to init SDL with OpenGL!" << std::endl;
		return -1;
	}

	PathtracerEngine engine;
	Window<PathtracerEngine> w("Pathtracer", 800, 600, engine);

	w.start();

	return 0;
}
