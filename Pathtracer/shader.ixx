module;

#include <gl/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <stdexcept>
#include <tuple>

export module shader;

import buffer;
import texture;

export class GpuProgram {
public:
	GpuProgram(std::string vertexGlsl, std::string fragmentGlsl) {
		GLuint vShader = _compileShader(GL_VERTEX_SHADER, vertexGlsl);
		GLuint fShader = _compileShader(GL_FRAGMENT_SHADER, fragmentGlsl);

		_makeProgram({ vShader, fShader });
	}

	GpuProgram(std::string computeGlsl) {
		GLuint cShader = _compileShader(GL_COMPUTE_SHADER, computeGlsl);

		_makeProgram({ cShader });
	}

	~GpuProgram() {
		glDeleteProgram(m_id);
	}

	GLuint id() {
		return m_id;
	}
private:
	GLuint m_id;

	GLuint _compileShader(GLenum type, std::string src) {
		GLuint id = glCreateShader(type);

		src = "#version 450\n" + src;

		auto cSrc = src.c_str();
		GLint len = static_cast<GLint>(src.size());

		glShaderSource(id, 1, &cSrc, &len);
		glCompileShader(id); 
		
		GLint isCompiled = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &isCompiled);

		if (isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
			std::string infoLog(maxLength, '\0');

			glGetShaderInfoLog(id, maxLength, &maxLength, infoLog.data());

			throw std::runtime_error("Shader compilation failed: " + infoLog);
		}

		return id;
	}

	void _makeProgram(std::vector<GLuint> shaders) {
		m_id = glCreateProgram();

		for (GLuint shaderId : shaders) {
			glAttachShader(m_id, shaderId);
		}

		glLinkProgram(m_id);

		GLint isLinked = 0;
		glGetProgramiv(m_id, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &maxLength);

			std::string infoLog(maxLength, '\0');
			glGetProgramInfoLog(m_id, maxLength, &maxLength, &infoLog[0]);

			throw std::runtime_error("Program link failed: " + infoLog);
		}

		for (GLuint shaderId : shaders) {
			glDetachShader(m_id, shaderId);
			glDeleteShader(shaderId);
		}
	}
};

export class GpuProgramState {
public:
	GpuProgramState() {}

	template<class T>
	GpuProgramState& storageBuffer(int location, GpuBuffer<T> *buffer) {
		m_buffers.push_back({ location, buffer->id(), GL_SHADER_STORAGE_BUFFER });

		return *this;
	}

	template<class T>
	GpuProgramState& uniformBuffer(int location, GpuBuffer<T> *buffer) {
		m_buffers.push_back({ location, buffer->id(), GL_UNIFORM_BUFFER });

		return *this;
	}

	template<class T>
	GpuProgramState& image(int location, GpuTexture<T> *texture) {
		m_imageUnits.push_back(texture->id());
		//m_ints.push_back({ location, m_imgUnits++ });

		return *this;
	}
	
	GpuProgramState& uniform(int location, int i) {
		m_ints.push_back({ location, i });

		return *this;
	}

	GpuProgramState& uniform(int location, float f) {
		m_floats.push_back({ location, f });

		return *this;
	}

	GpuProgramState& uniform(int location, glm::vec2 v2) {
		m_v2s.push_back({ location, v2 });

		return *this;
	}

	GpuProgramState& uniform(int location, glm::vec3 v3) {
		m_v3s.push_back({ location, v3 });

		return *this;
	}

	GpuProgramState& uniform(int location, glm::vec4 v4) {
		m_v4s.push_back({ location, v4 });

		return *this;
	}

	GpuProgramState& uniform(int location, glm::mat4 m4) {
		m_m4s.push_back({ location, m4 });

		return *this;
	}

	template<class T>
	GpuProgramState& uniform(int location, GpuTexture<T> *texture) {
		m_textureUnits.push_back(texture->id());
		m_ints.push_back({ location, m_texUnits++ });

		return *this;
	}

	void attach() {
		glBindTextures(0, static_cast<GLsizei>(m_textureUnits.size()), m_textureUnits.data());
		glBindImageTextures(0, static_cast<GLsizei>(m_imageUnits.size()), m_imageUnits.data());

		for (auto [binding, id, target] : m_buffers) {
			glBindBufferBase(target, binding, id);
		}

		for (auto [loc, v] : m_ints) {
			glUniform1i(loc, v);
		}

		for (auto [loc, v] : m_floats) {
			glUniform1f(loc, v);
		}

		for (auto [loc, v] : m_v2s) {
			glUniform2f(loc, v.x, v.y);
		}

		for (auto [loc, v] : m_v3s) {
			glUniform3f(loc, v.x, v.y, v.z);
		}

		for (auto [loc, v] : m_v4s) {
			glUniform4f(loc, v.x, v.y, v.z, v.w);
		}

		for (auto [loc, v] : m_m4s) {
			glUniformMatrix4fv(loc, 1, false, &v[0][0]);
		}
	}
private:
	int m_texUnits = 0;
	int m_imgUnits = 0;

	std::vector<std::tuple<int, int>> m_ints;
	std::vector<std::tuple<int, float>> m_floats;
	std::vector<std::tuple<int, glm::vec2>> m_v2s;
	std::vector<std::tuple<int, glm::vec3>> m_v3s;
	std::vector<std::tuple<int, glm::vec4>> m_v4s;
	std::vector<std::tuple<int, glm::mat4>> m_m4s;

	std::vector<GLuint> m_textureUnits;
	std::vector<GLuint> m_imageUnits;
	std::vector<std::tuple<int, GLuint, GLenum>> m_buffers;
};