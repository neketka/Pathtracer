module;

#include <gl/glew.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <stdexcept>

export module texture;

template<class T>
class GlTextureFormat;

export template<class T> class GpuTexture {
public:
	GpuTexture(int w, int h, bool depth) : m_w(w), m_h(h) {
		glCreateTextures(GL_TEXTURE_2D, 1, &m_id);
		glTextureStorage2D(m_id, 1, depth ? GlTextureFormat<T>::depthFormat : GlTextureFormat<T>::sizedFormat, w, h);
	}

	~GpuTexture() {
		glDeleteTextures(1, &m_id);
	}

	void setData(T *data, int x, int y, int w, int h) {
		glTextureSubImage2D(m_id, 0, x, y, w, h, GlTextureFormat<T>::baseFormat, GlTextureFormat<T>::type, data);
		glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void clear() {
		glm::vec4 v(0, 0, 0, 0);
		glClearTexImage(m_id, 0, GlTextureFormat<T>::baseFormat, GlTextureFormat<T>::type, &v);
	}

	int width() {
		return m_w;
	}

	int height() {
		return m_h;
	}

	bool depth() {
		return m_depth;
	}

	GLuint id() {
		return m_id;
	}
private:
	int m_w;
	int m_h;
	bool m_depth;
	GLuint m_id;
};

export class GpuRenderTarget {
public:
	GpuRenderTarget() : m_id(0) {}

	GpuRenderTarget(std::vector<GLuint> targets, GLuint depth, bool hasDepth) {
		glCreateFramebuffers(1, &m_id);
		int index = 0;

		std::vector<GLenum> attachments;
		for (GLuint target : targets) {
			auto attachment = GL_COLOR_ATTACHMENT0 + index++;
			glNamedFramebufferTexture(m_id, attachment, target, 0);
			attachments.push_back(attachment);
		}

		if (hasDepth) {
			glNamedFramebufferTexture(m_id, GL_DEPTH_ATTACHMENT, depth, 0);
		}

		glNamedFramebufferDrawBuffers(m_id, static_cast<GLsizei>(attachments.size()), attachments.data());
	}

	~GpuRenderTarget() {
		if (m_id != 0) {
			glDeleteFramebuffers(1, &m_id);
		}
	}

	GLuint id() {
		return m_id;
	}
private:
	GLuint m_id;
};

export class GpuRenderTargetBuilder {
public:
	template<class T>
	GpuRenderTargetBuilder& attachColor(GpuTexture<T> *tex) {
		m_colors.push_back(tex->id());

		return *this;
	}

	GpuRenderTargetBuilder& attachDepth(GpuTexture<int> *depthTex) {
		m_depth = depthTex->id();
		m_hasDepth = true;

		return *this;
	}

	GpuRenderTarget *build() {
		return new GpuRenderTarget(m_colors, m_depth, m_hasDepth);
	}
private:
	std::vector<GLuint> m_colors;
	GLuint m_depth;
	bool m_hasDepth = false;
};

template<>
class GlTextureFormat<glm::u8> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT24;
	const static GLenum sizedFormat = GL_R8;
	const static GLenum baseFormat = GL_RED;
	const static GLenum type = GL_UNSIGNED_BYTE;
};

template<>
class GlTextureFormat<glm::u8vec2> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT24;
	const static GLenum sizedFormat = GL_RG8;
	const static GLenum baseFormat = GL_RG;
	const static GLenum type = GL_UNSIGNED_BYTE;
};

template<>
class GlTextureFormat<glm::u8vec3> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT24;
	const static GLenum sizedFormat = GL_RGB8;
	const static GLenum baseFormat = GL_RGB;
	const static GLenum type = GL_UNSIGNED_BYTE;
};

template<>
class GlTextureFormat<glm::u8vec4> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT24;
	const static GLenum sizedFormat = GL_RGBA8;
	const static GLenum baseFormat = GL_RGBA;
	const static GLenum type = GL_UNSIGNED_BYTE;
};

template<>
class GlTextureFormat<glm::float32> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT32F;
	const static GLenum sizedFormat = GL_R32F;
	const static GLenum baseFormat = GL_RED;
	const static GLenum type = GL_FLOAT;
};

template<>
class GlTextureFormat<glm::vec2> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT32F;
	const static GLenum sizedFormat = GL_RG32F;
	const static GLenum baseFormat = GL_RG;
	const static GLenum type = GL_FLOAT;
};

template<>
class GlTextureFormat<glm::vec3> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT32F;
	const static GLenum sizedFormat = GL_RGB32F;
	const static GLenum baseFormat = GL_RGB;
	const static GLenum type = GL_FLOAT;
};

template<>
class GlTextureFormat<glm::vec4> {
public:
	const static GLenum depthFormat = GL_DEPTH_COMPONENT32F;
	const static GLenum sizedFormat = GL_RGBA32F;
	const static GLenum baseFormat = GL_RGBA;
	const static GLenum type = GL_FLOAT;
};