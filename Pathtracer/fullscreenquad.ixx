module;

#include <vector>
#include <string>
#include <glm/glm.hpp>

export module fullscreenquad;

import buffer;
import shader;
import pass;
import texture;

std::string vFullscreenQuad = R"glsl(

layout(location = 0) in vec2 pos;
layout(location = 0) out vec2 pos_out;

void main() {
	pos_out = pos * 0.5 + 0.5;

	gl_Position = vec4(pos, 0.5, 1.0);
}

)glsl";

std::string fFullscreenQuad = R"glsl(

layout(location = 0) out vec4 fragment;
layout(location = 0) in vec2 pos;

layout(location = 0) uniform sampler2D tex;

void main() {
	fragment = vec4(texture(tex, pos).rgb, 1.0);
}

)glsl";

export class FullScreenQuadPass {
public:
	FullScreenQuadPass() {
		m_buffer = new GpuBuffer<glm::vec2>(6);
		m_buffer->setData(m_square, 0, 0, 6);
		m_vao = new GpuVertexArray<glm::vec2>();
		m_vao->setBuffer(m_buffer);
		m_program = new GpuProgram(vFullscreenQuad, fFullscreenQuad);
	}

	~FullScreenQuadPass() {
		delete m_program;
		delete m_vao;
		delete m_buffer;
	}

	template<class T>
	void render(int w, int h, GpuTexture<T> *texture) {
		GpuRenderPass pass(
			m_program,
			std::forward<GpuProgramState>(GpuProgramState().uniform(0, texture)),
			std::forward<GpuRenderState>(GpuRenderState().viewport(0, 0, w, h).vertexArray(m_vao).memoryBarrier())
		);

		pass.attach();
		pass.drawTriangles(0, 6);
	}
private:
	std::vector<glm::vec2> m_square = {
		{ -1, -1 }, { 1, 1 }, { -1, 1 },
		{ -1, -1 }, { 1, -1 }, { 1, 1 }
	};

	GpuBuffer<glm::vec2>* m_buffer;
	GpuVertexArray<glm::vec2>* m_vao;
	GpuProgram* m_program;
};