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

layout(location = 1) uniform vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
layout(location = 2) uniform vec3 frustumTR = vec3(0.577, 0.577, 0.577);
layout(location = 3) uniform vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
layout(location = 4) uniform vec3 frustumBR = vec3(0.577, -0.577, 0.577);
layout(location = 5) uniform vec3 frustumOrigin = vec3(0.0);

const vec3 lightPos = vec3(2.0, 2.0, 9.0);

// Returns true and sets intersection position and normal, or returns false
bool planeRay(vec3 rayPos, vec3 rayDir, vec3 planePos, vec3 planeNormal, out vec3 pos, out vec3 normal) {
	float denom = dot(planeNormal, rayDir); 
    if (denom > 1e-6) { 
        vec3 p0l0 = planePos - rayPos; 
        float t = dot(p0l0, planeNormal) / denom; 
		pos = rayPos + rayDir * t;
		normal = -sign(denom) * planeNormal;
        return (t >= 0); 
    } 
	
 
    return false; 
}

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

	vec3 center = (boxMax + boxMin) * 0.5;
	vec3 sz = (boxMax - boxMin) * 0.5;
	float bias = 100000.0;

	normal = normalize(floor((pos - center) / abs(sz) * bias));

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

	vec3 rayPos = frustumOrigin;
	vec3 rayDir = getRayDir();

	vec3 hitPos;
	vec3 hitNormal;


	//bool hits = sphereRay(rayPos, rayDir, vec3(0.0, 0.0, 10.0), 2.0, hitPos, hitNormal);
	bool hits = planeRay(rayPos, rayDir, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), hitPos, hitNormal);
	//bool hits = boxRay(rayPos, rayDir, vec3(-1.0, -1.0, 9.0), vec3(1.0, 1.0, 11.0), hitPos, hitNormal);

	float lightFactor = max(0.0, dot(hitNormal, normalize(lightPos - hitPos)));
	vec3 surfaceColor = vec3(0.1) + vec3(1.0) * lightFactor;

	imageStore(target, pos, vec4(mix(vec3(0.0), surfaceColor, int(hits)), 1.0));
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
		m_w = w;
		m_h = h;
	}

	GpuTexture<glm::vec4>* getColor() {
		assert(m_color);
		return m_color;
	}

	GpuTexture<float>* getDepth() {
		assert(m_depth);
		return m_depth;
	}

	int w() {
		return m_w;
	}

	int h() {
		return m_h;
	}
private:
	int m_w, m_h;
	GpuTexture<glm::vec4>* m_color = nullptr;;
	GpuTexture<float>* m_depth = nullptr;
};

export class PathtracingPass {
public:
	PathtracingPass() {
		m_program = new GpuProgram(cPathtracing);
	}

	void render(glm::mat4 viewMatrix, PathtracingBuffer* target) {
		float invAspect = target->h() / static_cast<float>(target->w());

		glm::mat4 mvp =
			glm::scale(glm::mat4(1.0), glm::vec3(1.f, invAspect, 1.f)) * viewMatrix;

		glm::vec3 frustumTL = glm::vec4(-0.577, 0.577, 0.577, 0.f) * mvp;
		glm::vec3 frustumTR = glm::vec4(0.577, 0.577, 0.577, 0.f) * mvp;
		glm::vec3 frustumBL = glm::vec4(-0.577, -0.577, 0.577, 0.f) * mvp;
		glm::vec3 frustumBR = glm::vec4(0.577, -0.577, 0.577, 0.f) * mvp;

		GpuComputePass pass(
			m_program,
			std::forward<GpuProgramState>(
				GpuProgramState()
					.image(0, target->getColor())
					.uniform(1, frustumTL)
					.uniform(2, frustumTR)
					.uniform(3, frustumBL)
					.uniform(4, frustumBR)
					.uniform(5, glm::vec3(mvp[3]))
			)
		);

		pass.attach();
		pass.dispatch(target->w(), target->h(), 1);
	}
private:
	GpuProgram* m_program;
};