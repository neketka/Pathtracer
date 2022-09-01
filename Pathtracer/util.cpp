

/*
export enum class TextureFormat {
	R32F, RG32F, RGB32F, RGBA32F, Depth32F, Depth24I, RGB8UI, RGBA8UI
};

GLenum toSizedFormat(TextureFormat format) {
	static std::vector<GLenum> mapping = {
		GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT24, GL_RGB8UI, GL_RGBA8UI
	};

	return mapping[static_cast<int>(format)];
}

GLenum toBaseFormat(TextureFormat format) {
	static std::vector<GLenum> mapping = {
		GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_RGB, GL_RGBA
	};

	return mapping[static_cast<int>(format)];
}

GLenum toDataType(TextureFormat format) {
	static std::vector<GLenum> mapping = {
		GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE
	};

	return mapping[static_cast<int>(format)];
}
*/