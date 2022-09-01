module;

#include <gl/glew.h>

export module pass;

import buffer;
import texture;
import shader;

export class GpuRenderState {
public:
	GpuRenderState() {}

	GpuRenderState& depthTest() {
		m_depthTest = true;
		return *this;
	}

	GpuRenderState& rasterizerDiscard() {
		m_rasterizerDiscard = true;
		return *this;
	}

	GpuRenderState& viewport(int x, int y, int w, int h) {
		m_x = x; m_y = y; m_w = w; m_h = h;
		return *this;
	}

	GpuRenderState& renderTarget(GpuRenderTarget renderTarget) {
		m_fbo = renderTarget.id();
		return *this;
	}

	template<class T>
	GpuRenderState& vertexArray(GpuVertexArray<T> *array) {
		m_vao = array->id();
		return *this;
	}

	void attach() {
		if (m_depthTest) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}

		if (m_rasterizerDiscard) {
			glEnable(GL_RASTERIZER_DISCARD);
		} else {
			glDisable(GL_RASTERIZER_DISCARD);
		}

		glViewport(m_x, m_y, m_w, m_h);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glBindVertexArray(m_vao);
	}
private:
	bool m_depthTest = false, m_rasterizerDiscard = false;
	int m_x = 0, m_y = 0, m_w = 800, m_h = 600;
	GLuint m_fbo = 0, m_vao = 0;
};

export class GpuRenderPass {
public:
	GpuRenderPass(GpuProgram *program, GpuProgramState&& data, GpuRenderState&& state) : m_program(program->id()), m_data(data), m_state(state) {
	}

	void attach() {
		glUseProgram(m_program);
		m_data.attach();
		m_state.attach();
	}

	void drawTriangleElements(int start, int count) {
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, reinterpret_cast<void *>(start * sizeof(int)));
	}

	void drawTriangles(int start, int count) {
		glDrawArrays(GL_TRIANGLES, start, count);
	}
private:
	GLuint m_program;
	GpuProgramState m_data;
	GpuRenderState m_state;
};

export class GpuComputePass {
public:
	GpuComputePass(GpuProgram& program, GpuProgramState& data) : m_program(program), m_data(data) {
	}

	void attach() {
		glUseProgram(m_program.id());
		m_data.attach();
	}

	void dispatch(int x, int y, int z) {
		glDispatchCompute(x, y, z);
	}
private:
	GpuProgram& m_program;
	GpuProgramState& m_data;
};