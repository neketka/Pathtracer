module;

#include <iostream>

export module engine;

export class PathtracerEngine {
public:
	PathtracerEngine() {}

	void start() {
	}

	void stop() {
	}

	void tick(float deltaT, float fps) {
		if (m_fpsClock >= 1.0) {
			std::cout << "Avg. FPS: " << fps << ", Avg. frametime (ms): " << 1000.0 / fps << std::endl;
			m_fpsClock = 0;
		}
		m_fpsClock += deltaT;
	}

	void render(float deltaT, float fps) {
	}
private:
	float m_fpsClock = 0.0;
};