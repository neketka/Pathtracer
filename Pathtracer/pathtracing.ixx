module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <random>

export module pathtracing;

import texture;
import pass;
import buffer;
import shader;
import gpuscenesystem;

export class PathtracingBuffer {
public:
	PathtracingBuffer() {
	}

	~PathtracingBuffer() {
		delete m_color;
		delete m_depth;
	}

	void resize(int w, int h) {
		if (m_color) {
			delete m_color;
			delete m_depth;
		}

		m_color = new GpuTexture<glm::vec4>(w, h, false);
		m_depth = new GpuTexture<float>(w, h, true);
		m_w = w;
		m_h = h;
		m_aspect = m_w / static_cast<float>(m_h);
	}

	GpuTexture<glm::vec4>* getColor() {
		assert(m_color);
		return m_color;
	}

	GpuTexture<float>* getDepth() {
		assert(m_depth);
		return m_depth;
	}

	int w() {
		return m_w;
	}

	int h() {
		return m_h;
	}

	float aspect() {
		return m_aspect;
	}
private:
	int m_w, m_h;
	float m_aspect;
	GpuTexture<glm::vec4>* m_color = nullptr;
	GpuTexture<float>* m_depth = nullptr;
};

export class PathtracingConfig {
public:
	int bounces = 2;
	int shadowSamples = 1;
	float jitter = 0.5f;
	float lightRadius = 1.f;
	bool progressive = true;
};

export class PathtracingPass {
public:
	PathtracingPass(GpuProgram *pathtracer) : m_dist(0.0, 1.0) {
		m_program = pathtracer;
	}

	void render(glm::mat4 viewMatrix, PathtracingBuffer* target, GpuBuffer<GpuTri> *gpuTris, GpuBuffer<GpuBvhNode> *gpuBvh, GpuBuffer<GpuMat> *gpuMats) {
		glm::mat4 mvp = viewMatrix * glm::mat4(
			target->aspect(), 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		);

		if (mvp != m_mvp) {
			m_samples = 0;
			m_mvp = mvp;
		}

		GpuComputePass pass(
			m_program,
			std::forward<GpuProgramState>(
				GpuProgramState()
					.image(0, target->getColor())
					.uniform(0, mvp)
					.uniform(1, m_config.progressive ? m_samples++ : 0)
					.uniform(2, m_config.progressive ? m_dist(m_gen) : 0)
					.uniform(3, m_config.jitter)
					.uniform(4, m_samples == 1 ? glm::min(1, m_config.bounces) : m_config.bounces)
					.uniform(5, m_config.lightRadius)
					.uniform(6, glm::vec3(0.f, 3.f, 0.f))
					.uniform(7, glm::vec3(1.f, 1.f, 1.f) * 10.f)
					.storageBuffer(0, gpuTris)
					.storageBuffer(1, gpuBvh)
					.uniformBuffer(0, gpuMats)
			)
		);

		pass.attach();
		pass.dispatch(target->w(), target->h(), 1);
	}

	void setConfig(PathtracingConfig& config) {
		m_samples = 0;
		m_config = config;
	}
private:
	std::uniform_real_distribution<float> m_dist;
	std::default_random_engine m_gen;

	PathtracingConfig m_config;

	glm::mat4 m_mvp;
	int m_samples = 0;
	GpuProgram* m_program;
};
