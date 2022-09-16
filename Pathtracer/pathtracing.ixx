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

layout(location = 0) uniform vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
layout(location = 1) uniform vec3 frustumTR = vec3(0.577, 0.577, 0.577);
layout(location = 2) uniform vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
layout(location = 3) uniform vec3 frustumBR = vec3(0.577, -0.577, 0.577);
layout(location = 4) uniform vec3 frustumOrigin = vec3(0.0);

const vec3 lightPos = vec3(2.0, 2.0, 9.0);

// Returns true and sets intersection position and normal, or returns false
bool planeRay(vec3 rayPos, vec3 rayDir, vec3 planePos, vec3 planeNormal, out vec3 pos, out vec3 normal, out float t) {
	float denom = dot(planeNormal, rayDir); 

    if (abs(denom) > 1e-6) { 
        vec3 p0l0 = planePos - rayPos; 
        t = dot(p0l0, planeNormal) / denom; 
		pos = rayPos + rayDir * t;
		normal = -sign(denom) * planeNormal;
        return (t >= 0); 
    } 
	
    return false; 
}

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(vec3 rayPos, vec3 rayDir, vec3 spherePos, float sphereRadius, out vec3 pos, out vec3 normal, out float t) {
	float R2 = sphereRadius * sphereRadius;
    vec3 L = spherePos - rayPos;
    float tca = dot(L, rayDir);

    float D2 = dot(L, L) - tca * tca;
    if(D2 > R2) return false;
    float thc = sqrt(R2 - D2);
    float t0 = tca - thc;
    float t1 = tca + thc;
	
	t = t0;
    pos = t0 * rayDir + rayPos;
	normal = normalize(pos - spherePos);
    return true;
}

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRay(vec3 rayPos, vec3 rayDir, vec3 boxMin, vec3 boxMax, out vec3 pos, out vec3 normal, out float t) {
	float tmin = 0.0, tmax = 1.0 / 0.0;
	vec3 dirInv = vec3(1.0) / rayDir;

    for (int i = 0; i < 3; ++i) {
        float t1 = (boxMin[i] - rayPos[i]) * dirInv[i];
        float t2 = (boxMax[i] - rayPos[i]) * dirInv[i];

        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }

	t = tmin;
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

void findClosest(
	bool hit, vec3 hitPos, vec3 hitNormal, float hitT, 
	inout vec3 closestPos, inout vec3 closestNormal, inout float closestT, inout bool anyHit
) {
	anyHit = anyHit || hit;

	int mixVal = int(hit && hitT < closestT);
	closestPos = mix(closestPos, hitPos, mixVal);
	closestNormal = mix(closestNormal, hitNormal, mixVal);
	closestT = mix(closestT, hitT, mixVal);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	vec3 rayPos = frustumOrigin;
	vec3 rayDir = normalize(getRayDir());

	vec3 closestPos = vec3(0.0, 0.0, 0.0);
	vec3 closestNormal = vec3(0.0, 0.0, 0.0);
	float closestT = 100000000.0;
	bool anyHit = false;

	vec3 hitPos = vec3(0.0, 0.0, 0.0);
	vec3 hitNormal = vec3(0.0, 0.0, 0.0);
	float hitT = 100000000.0;

	findClosest(
		planeRay(rayPos, rayDir, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), hitPos, hitNormal, hitT),
		hitPos, hitNormal, hitT, closestPos, closestNormal, closestT, anyHit
	);

	findClosest(
		boxRay(rayPos, rayDir, vec3(1.0, 1.0, 9.0), vec3(2.0, 2.0, 10.0), hitPos, hitNormal, hitT),
		hitPos, hitNormal, hitT, closestPos, closestNormal, closestT, anyHit
	);

	findClosest(
		sphereRay(rayPos, rayDir, vec3(-3.0, 0.0, 10.0), 1.0, hitPos, hitNormal, hitT),
		hitPos, hitNormal, hitT, closestPos, closestNormal, closestT, anyHit
	);

	float lightFactor = max(0.0, dot(closestNormal, normalize(lightPos - closestPos)));
	vec3 surfaceColor = vec3(0.1) + vec3(1.0) * lightFactor;

	imageStore(target, pos, vec4(mix(vec3(0.0), surfaceColor, int(anyHit)), 1.0));
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
					.uniform(0, frustumTL)
					.uniform(1, frustumTR)
					.uniform(2, frustumBL)
					.uniform(3, frustumBR)
					.uniform(4, glm::vec3(mvp[3]))
			)
		);

		pass.attach();
		pass.dispatch(target->w(), target->h(), 1);
	}
private:
	GpuProgram* m_program;
};