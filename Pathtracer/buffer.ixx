module;

#include <gl/glew.h>
#include <glm/glm.hpp>

#include <vector>

export module buffer;

export class StructField {
public:
	bool isFloat = true;
	int components = 0;
	int typeSize = 0;
};

export template<class T>
std::vector<T> concatVectors(std::vector<std::vector<T>> vecs) {
	std::vector<T> concated;

	for (std::vector<T>& vec : vecs) {
		concated.insert(concated.end(), vec.begin(), vec.end());
	}

	return concated;
}

export template<class T>
class StructFormat;

export template<>
class StructFormat<int> {
public:
	static std::vector<StructField> fields() {
		return {
			{ .isFloat = false, .components = 1, .typeSize = sizeof(int) }
		};
	}
};

export template<>
class StructFormat<float> {
public:
	static std::vector<StructField> getFields() {
		return {
			{ .isFloat = true, .components = 1, .typeSize = sizeof(float) }
		};
	}
};

export template<>
class StructFormat<glm::vec2> {
public:
	static std::vector<StructField> getFields() {
		return {
			{ .isFloat = true, .components = 2, .typeSize = sizeof(float) }
		};
	}
};

export template<>
class StructFormat<glm::vec3> {
public:
	static std::vector<StructField> getFields() {
		return {
			{ .isFloat = true, .components = 3, .typeSize = sizeof(float) }
		};
	}
};

export template<>
class StructFormat<glm::vec4> {
public:
	static std::vector<StructField> getFields() {
		return {
			{ .isFloat = true, .components = 4, .typeSize = sizeof(float) }
		};
	}
};

export template<class T>
class GpuBuffer {
public:
	GpuBuffer(unsigned int size) : m_size(size) {
		glCreateBuffers(1, &m_id);
		glNamedBufferStorage(m_id, sizeof(T) * size);
	}

	void setData(std::vector<T>& src, int srcIndex, int destIndex, int size) {
		glNamedBufferSubData(m_id, destIndex * sizeof(T), size * sizeof(T), src.data() + srcIndex);
	}

	inline GLuint id() {
		return m_id;
	}

	inline unsigned int size() {
		return m_size;
	}
private:
	GLuint m_id;
	unsigned int m_size;
};

export template<class T>
class GpuVertexArray {
public:
	GpuVertexArray() {
		glCreateVertexArrays(1, &m_id);
		_setFormat();
	}

	void setBuffer(GpuBuffer<T> buf) {
		glVertexArrayVertexBuffer(m_id, 0, buf.id(), 0, stride);
	}

	void setIndexBuffer(GpuBuffer<int> buf) {
		glVertexArrayElementBuffer(m_id, buf.id());
	}

	inline GLuint id() {
		return m_id;
	}
private:
	GLuint m_id;
	int m_stride;

	void _setFormat() {
		auto format = StructFormat<T>::format();
		int offset = 0;
		int index = 0;
		for (StructField fmt : format) {
			if (fmt.isFloat)
				glVertexArrayAttribFormat(m_id, index, attrib.components, GL_FLOAT, false, offset);
			else
				glVertexArrayAttribIFormat(m_id, index, attrib.components, GL_INT, offset);
			glEnableVertexArrayAttrib(m_id, index);

			offset += fmt.components * fmt.typeSize;
			++index;
		}

		m_stride = offset;
	}
};