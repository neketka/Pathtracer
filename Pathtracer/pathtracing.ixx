module;

#include <glm/glm.hpp>
#include <string>

export module pathtracing;

import texture;
import pass;
import shader;

std::string cPathtracing = R"glsl(

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f) uniform image2D target;

const vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
const vec3 frustumTR = vec3(0.577, 0.577, 0.577);
const vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
const vec3 frustumBR = vec3(0.577, -0.577, 0.577);
const vec3 frustumOrigin = vec3(0.0, 0.0, 0.0);

const vec3 spherePos = vec3(0.0, 0.0, 10.0);
const float sphereRadius = 2.0;

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(vec3 rayPos, vec3 rayDir, vec3 spherePos, float sphereRadius, out vec3 pos, out vec3 normal) {
	return false;
}

vec3 getRayDir() {
	vec2 factor = vec2(gl_GlobalInvocationID.xy) / vec2(gl_NumWorkGroups.xy);

	vec3 top = mix(frustumTL, frustumTR, factor.x);
	vec3 bottom = mix(frustumBL, frustumBR, factor.x);

	return mix(bottom, top, factor.y);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	vec3 rayPos = frustumOrigin;
	vec3 rayDir = getRayDir();

	vec3 hitPos;
	vec3 hitNormal;

	bool hitsSphere = sphereRay(rayPos, rayDir, spherePos, sphereRadius, hitPos, hitNormal);

	vec3 pixelColor = mix(vec3(0.0), vec3(1.0), int(hitsSphere));

	imageStore(target, pos, vec4(pixelColor, 1.0));
}

)glsl";

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
	}

	GpuTexture<glm::vec4>* getColor() {
		assert(m_color);
		return m_color;
	}

	GpuTexture<float>* getDepth() {
		assert(m_depth);
		return m_depth;
	}
private:
	GpuTexture<glm::vec4>* m_color = nullptr;;
	GpuTexture<float>* m_depth = nullptr;
};

export class PathtracingPass {
public:
	PathtracingPass() {
		m_program = new GpuProgram(cPathtracing);
	}

	void render(int w, int h, PathtracingBuffer *target) {
		GpuComputePass pass(m_program, std::forward<GpuProgramState>(GpuProgramState().image(0, target->getColor())));

		pass.attach();
		pass.dispatch(w, h, 1);
	}
private:
	GpuProgram* m_program;
};