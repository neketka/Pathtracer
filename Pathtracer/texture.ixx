module;

#include <gl/glew.h>
#include <vector>

export module texture;

export enum class TextureFormat {
	R32F, RG32F, RGB32F, RGBA32F, Depth32F, Depth24I, RGB8UI, RGBA8UI
};

GLenum toFormat(TextureFormat format) {
	static std::vector<GLenum> mapping = {
		GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24, GL_RGB8UI, GL_RGBA8UI
	};

	return mapping[static_cast<int>(format)];
}

export class GpuTexture {
public:
	GpuTexture(int w, int h, TextureFormat format) : m_w(w), m_h(h), m_format(format) {
		glCreateTextures(GL_TEXTURE_2D, 1, &m_id);
		glTextureStorage2D(m_id, 1, toFormat(format), w, h);
	}

	void setData(void *data, int x, int y, int w, int h) {
		//glTextureSubImage2D(m_id, 0, x, y, w, h, toFormat(format), );
	}

	int width() {
		return m_w;
	}

	int height() {
		return m_h;
	}

	TextureFormat format() {
		return m_format;
	}
private:
	int m_w;
	int m_h;
	TextureFormat m_format;
	GLuint m_id;


};