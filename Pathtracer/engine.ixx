module;

#include <glm/glm.hpp>

#include <vector>
#include <string>

export module engine;

import pathtracing;
import fullscreenquad;

export class PathtracerEngine {
public:
	PathtracerEngine() {}

	void start() {
		m_fQuad = new FullScreenQuadPass;
		m_pathtracing = new PathtracingPass;
		m_pBuffer = new PathtracingBuffer;
	}

	void stop() {
		delete m_fQuad;
		delete m_pathtracing;
		delete m_pBuffer;
	}

	void resize(int w, int h) {
		m_w = w;
		m_h = h;

		m_pBuffer->resize(w, h);
	}

	void tick(float deltaT, float fps) {
	}

	void render(float deltaT, float fps) {
		m_pathtracing->render(m_w, m_h, m_pBuffer);
		m_fQuad->render(m_w, m_h, m_pBuffer->getColor());
	}
private:
	int m_w = 800, m_h = 600;

	FullScreenQuadPass *m_fQuad;
	PathtracingPass *m_pathtracing;
	PathtracingBuffer *m_pBuffer;
};