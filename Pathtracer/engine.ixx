module;

#include <glm/glm.hpp>

#include <vector>
#include <string>

export module engine;

import fullscreenquad;
import pathtracing;
import texture;

export class PathtracerEngine {
public:
	PathtracerEngine() {}

	void start() {
		m_fQuad = new FullScreenQuadPass;
		m_pathtracing = new PathtracingPass;
	}

	void stop() {
		delete m_fQuad;
		delete m_pathtracing;
		delete m_texture;
	}

	void resize(int w, int h) {
		m_w = w;
		m_h = h;

		if (m_texture) {
			delete m_texture;
		}
		m_texture = new GpuTexture<glm::vec4>(w, h, false);
	}

	void tick(float deltaT, float fps) {
	}

	void render(float deltaT, float fps) {
		m_pathtracing->render(m_w, m_h, m_texture);
		m_fQuad->render(m_w, m_h, m_texture);
	}
private:
	int m_w = 800, m_h = 600;

	FullScreenQuadPass *m_fQuad;
	PathtracingPass *m_pathtracing;

	GpuTexture<glm::vec4> *m_texture;
};