module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

export module pathtracing;

import texture;
import pass;
import shader;

std::string cPathtracing = R"glsl(

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D target;
layout(location = 1) uniform mat4 mvpMatrix = mat4(1.0);

const vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
const vec3 frustumTR = vec3(0.577, 0.577, 0.577);
const vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
const vec3 frustumBR = vec3(0.577, -0.577, 0.577);

const vec3 spherePos = vec3(0.0, 0.0, 10.0);
const float sphereRadius = 2.0;

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(vec3 rayPos, vec3 rayDir, vec3 spherePos, float sphereRadius, out vec3 pos, out vec3 normal) {
	return false;
}

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRay(vec3 rayPos, vec3 rayDir, vec3 boxMin, vec3 boxMax, out vec3 pos, out vec3 normal) {
	float tmin = 0.0, tmax = 1.0 / 0.0;
	vec3 dirInv = vec3(1.0) / rayDir;

    for (int i = 0; i < 3; ++i) {
        float t1 = (boxMin[i] - rayPos[i]) * dirInv[i];
        float t2 = (boxMax[i] - rayPos[i]) * dirInv[i];

        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }

	pos = tmin * rayDir + rayPos;

    return tmin < tmax;
}

vec3 getRayDir() {
	vec2 factor = vec2(gl_GlobalInvocationID.xy) / vec2(gl_NumWorkGroups.xy);

	vec3 top = mix(frustumTL, frustumTR, factor.x);
	vec3 bottom = mix(frustumBL, frustumBR, factor.x);

	return mix(bottom, top, factor.y);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	vec3 rayPos = mvpMatrix[3].xyz;
	vec3 rayDir = (vec4(getRayDir(), 0.0) * mvpMatrix).xyz;

	vec3 hitPos;
	vec3 hitNormal;

	//bool hits = sphereRay(rayPos, rayDir, spherePos, sphereRadius, hitPos, hitNormal);
	bool hits = boxRay(rayPos, rayDir, vec3(-1.0, -1.0, 9.0), vec3(1.0, 1.0, 11.0), hitPos, hitNormal);

	vec3 pixelColor = mix(vec3(0.0), vec3(1.0), int(hits));

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
		m_invAspect = h / static_cast<float>(w);
	}

	float getInvAspect() {
		return m_invAspect;
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
	float m_invAspect = 1.0;
	GpuTexture<glm::vec4>* m_color = nullptr;;
	GpuTexture<float>* m_depth = nullptr;
};

export class PathtracingPass {
public:
	PathtracingPass() {
		m_program = new GpuProgram(cPathtracing);
	}

	void render(int w, int h, PathtracingBuffer *target) {
		glm::mat4 mvp = 
			glm::translate(glm::mat4(1.0), glm::vec3(0.f, 0.f, -10.f)) *
			glm::rotate(glm::mat4(1.0), 0.2f, glm::vec3(0.0, 1.0, 0.0)) *
			glm::rotate(glm::mat4(1.0), 0.2f, glm::vec3(1.0, 0.0, 0.0)) *
			glm::rotate(glm::mat4(1.0), 0.f, glm::vec3(0.0, 0.0, 1.0)) *
			glm::scale(glm::mat4(1.0), glm::vec3(1.f, target->getInvAspect(), 1.f));

		GpuComputePass pass(
			m_program,
			std::forward<GpuProgramState>(
				GpuProgramState().image(0, target->getColor()).uniform(1, mvp)
			)
		);

		pass.attach();
		pass.dispatch(w, h, 1);
	}
private:
	GpuProgram* m_program;
};