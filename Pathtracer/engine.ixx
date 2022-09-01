module;

#include <glm/glm.hpp>

#include <vector>
#include <string>

export module engine;

import buffer;
import shader;
import pass;

export class PathtracerEngine {
public:
	PathtracerEngine() {}

	void start() {
		m_buffer = new GpuBuffer<glm::vec2>(6);
		m_buffer->setData(m_square, 0, 0, 6);
		m_vao = new GpuVertexArray<glm::vec2>();
		m_vao->setBuffer(m_buffer);
		m_program = new GpuProgram(vShader, fShader);
	}

	void stop() {
		delete m_program;
		delete m_vao;
		delete m_buffer;
	}

	void resize(int w, int h) {
		m_w = w;
		m_h = h;
	}

	void tick(float deltaT, float fps) {
	}

	void render(float deltaT, float fps) {
		GpuRenderPass pass(
			m_program,
			std::forward<GpuProgramState>(GpuProgramState().setUniform(0, m_time)),
			std::forward<GpuRenderState>(GpuRenderState().viewport(0, 0, m_w, m_h).vertexArray(m_vao))
		);

		pass.attach();
		pass.drawTriangles(0, 6);

		m_time += deltaT;
	}
private:
	std::vector<glm::vec2> m_square = { 
		{ -1, -1 }, { 1, 1 }, { -1, 1 }, 
		{ -1, -1 }, { 1, -1 }, { 1, 1 }
	};
	
	int m_w = 800, m_h = 600;
	float m_time = 0.0;
	
	GpuBuffer<glm::vec2> *m_buffer;
	GpuVertexArray<glm::vec2> *m_vao;
	GpuProgram *m_program;

	std::string vShader = R"glsl(

layout(location = 0) in vec2 pos;
layout(location = 0) out vec2 pos_out;

void main() {
	pos_out = pos;

	gl_Position = vec4(pos, 0.5, 1.0);
}

)glsl";

	std::string fShader = R"glsl(

layout(location = 0) out vec4 fragment;

layout(location = 0) in vec2 pos;
layout(location = 0) uniform float time;

void main() {
	vec2 npos = (pos + vec2(1.0)) * 0.5;
	vec2 t = time * 4.0 * vec2(-1.0, 1.0);
	float factor = (sin(t.x + npos.x * 64.0) + 1.0) * (sin(t.y + npos.y * 64.0) + 1.0) * 0.25;

	fragment = vec4(mix(vec3(0.0, 0.0, 1.0), vec3(0.1, 0.9, 0.1), factor), 1.0);
}

)glsl";
};