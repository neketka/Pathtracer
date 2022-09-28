module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <vector>
#include <string>

export module engine;

import pathtracing;
import fullscreenquad;
import window;
import transforms;

export class PathtracerEngine {
public:
	PathtracerEngine() {}

	void start(Window<PathtracerEngine>& window) {
		m_fQuad = new FullScreenQuadPass;
		m_pathtracing = new PathtracingPass;
		m_pBuffer = new PathtracingBuffer;
		m_window = &window;
		setConfig();
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

	void key(int id, bool keyDown) {
		int bit = keyDown ? 1 : 0;

		if (id == 26) {
			m_movement = m_movement & 0b1110 | keyDown;
		} else if (id == 4) {
			m_movement = m_movement & 0b1101 | keyDown << 1;
		} else if (id == 22) {
			m_movement = m_movement & 0b1011 | keyDown << 2;
		} else if (id == 7) {
			m_movement = m_movement & 0b0111 | keyDown << 3;
		} 
		
		if (id == 46 && !keyDown) {
			m_configStage++;
			setConfig();
		} else if (id == 45 && !keyDown) {
			m_configStage--;
			setConfig();
		}
	}

	void mouseButton(bool leftButton, bool rightButton, bool buttonDown) {
		if (leftButton && !buttonDown) {
			m_mouseCapture = !m_mouseCapture;
			m_window->captureMouse(m_mouseCapture);
		}
	}

	void mouseMove(int x, int y, int dX, int dY) {
		if (m_mouseCapture) {
			m_rotX += -dY * 0.003f;
			m_rotY += -dX * 0.003;

			m_rotX = glm::clamp(m_rotX, -1.57f, 1.57f);
		}
	}

	void tick(float deltaT, float fps) {
		glm::mat4 view = cameraViewMatrix(m_pos, glm::vec3(m_rotX, m_rotY, 0.f));

		glm::vec3 right = glm::vec3(glm::row(view, 0)) * deltaT * 2.4f;
		glm::vec3 forward = glm::vec3(glm::row(view, 2)) * deltaT * 2.4f;

		if (m_mouseCapture) {
			if (m_movement & 0b0001) {
				m_pos += forward;
			} else if (m_movement & 0b0100) {
				m_pos -= forward;
			}
			
			if (m_movement & 0b0010) {
				m_pos -= right;
			} else if (m_movement & 0b1000) {
				m_pos += right;
			}
		}
	}

	void render(float deltaT, float fps) {
		glm::mat4 view = cameraViewMatrix(m_pos, glm::vec3(m_rotX, m_rotY, 0.f));
		m_pathtracing->render(view, m_pBuffer);
		m_fQuad->render(m_w, m_h, m_pBuffer->getColor());
	}
private:
	int m_movement = 0; // 0: none, 1: W, 2: A, 4: S, D: 8 and any bitwise combination
	int m_w = 800, m_h = 600;
	glm::vec3 m_pos = glm::vec3(0, 2, 0);
	float m_rotX = -0.2f, m_rotY = 0.01f;
	bool m_mouseCapture = false;

	int m_configStage = 0;

	void setConfig() {
		PathtracingConfig config;
		
		m_configStage = glm::clamp<int>(m_configStage, 0, 8);
		switch (m_configStage) {
		case 0:
			config.bounces = 0;
			config.jitter = 0;
			config.shadowSamples = 1;
			config.lightRadius = 0.f;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=1; LightRadius=0.0;");
			break;
		case 1:
			config.bounces = 0;
			config.jitter = 0;
			config.shadowSamples = 1;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=1; LightRadius=1.0;");
			break;
		case 2:
			config.bounces = 0;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=0; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 3:
			config.bounces = 1;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=1; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 4:
			config.bounces = 2;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=2; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 5:
			config.bounces = 3;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=3; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 6:
			config.bounces = 4;
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=4; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 7:
			config.jitter = 0;
			m_window->setTitle("Pathtracer: Bounces=5; JitterAA=0; ShadowSamples=2; LightRadius=1.0;");
			break;
		case 8:
			m_window->setTitle("Pathtracer: Bounces=5; JitterAA=0.5; ShadowSamples=2; LightRadius=1.0;");
			break;
		}

		m_pathtracing->setConfig(config);
	}

	Window<PathtracerEngine> *m_window;
	FullScreenQuadPass *m_fQuad;
	PathtracingPass *m_pathtracing;
	PathtracingBuffer *m_pBuffer;
};