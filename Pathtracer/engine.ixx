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

	Window<PathtracerEngine> *m_window;
	FullScreenQuadPass *m_fQuad;
	PathtracingPass *m_pathtracing;
	PathtracingBuffer *m_pBuffer;
};