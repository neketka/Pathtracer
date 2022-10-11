module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

export module movementsystem;

import extengine;
import window;
import transforms;
import <typeinfo>;

export class MovementSystem : public ExtEngineSystem {
public:
	MovementSystem() {}

	virtual void start(ExtEngine& engine) override {
		m_window = &engine.getWindow();
	}

	virtual void key(int id, bool keyDown) override {
		int bit = keyDown ? 1 : 0;

		if (id == 26) {
			m_movement = m_movement & 0b1110 | bit;
		}
		else if (id == 4) {
			m_movement = m_movement & 0b1101 | bit << 1;
		}
		else if (id == 22) {
			m_movement = m_movement & 0b1011 | bit << 2;
		}
		else if (id == 7) {
			m_movement = m_movement & 0b0111 | bit << 3;
		}
	}

	virtual void mouseButton(bool leftButton, bool rightButton, bool buttonDown) override {
		if (leftButton && !buttonDown) {
			m_mouseCapture = !m_mouseCapture;
			m_window->captureMouse(m_mouseCapture);
		}
	}

	virtual void mouseMove(int x, int y, int dX, int dY) override {
		if (m_mouseCapture) {
			m_rotX += -dY * 0.003f;
			m_rotY += -dX * 0.003f;

			m_rotX = glm::clamp(m_rotX, -1.57f, 1.57f);
		}
	}

	virtual void tick(float deltaT, float fps) override {
		glm::mat4 view = getViewMatrix();

		glm::vec3 right = glm::vec3(glm::row(view, 0)) * deltaT * 2.4f;
		glm::vec3 forward = glm::vec3(glm::row(view, 2)) * deltaT * 2.4f;

		if (m_mouseCapture) {
			if (m_movement & 0b0001) {
				m_pos += forward;
			}
			else if (m_movement & 0b0100) {
				m_pos -= forward;
			}

			if (m_movement & 0b0010) {
				m_pos -= right;
			}
			else if (m_movement & 0b1000) {
				m_pos += right;
			}
		}
	}

	float getRotX() {
		return m_rotX;
	}

	float getRotY() {
		return m_rotY;
	}

	glm::vec3 getPos() {
		return m_pos;
	}

	glm::mat4 getViewMatrix() {
		return cameraViewMatrix(m_pos, glm::vec3(m_rotX, m_rotY, 0.f)
		);
	}
private:
	int m_movement = 0; // 0: none, 1: W, 2: A, 4: S, D: 8 and any bitwise combination
	glm::vec3 m_pos = glm::vec3(0, 1, 2);
	float m_rotX = 0.f, m_rotY = -3.14f;
	bool m_mouseCapture = false;

	Window<ExtEngine>* m_window;
};