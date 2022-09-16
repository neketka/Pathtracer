module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

export module pathtracing;

import texture;
import pass;
import shader;

std::string cPathtracing = R"glsl(

struct Ray {
	vec3 pos;
	vec3 dir;
};

struct IntersectionInfo {
	vec3 pos;
	vec3 normal;
	float t;
};

struct Sphere {
	vec3 center;
	float radius;
};

struct Plane {
	vec3 origin;
	vec3 normal;
};

struct Box {
	vec3 min;
	vec3 max;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D target;

layout(location = 0) uniform vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
layout(location = 1) uniform vec3 frustumTR = vec3(0.577, 0.577, 0.577);
layout(location = 2) uniform vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
layout(location = 3) uniform vec3 frustumBR = vec3(0.577, -0.577, 0.577);
layout(location = 4) uniform vec3 frustumOrigin = vec3(0.0);

const vec3 lightPos = vec3(2.0, 5.0, 10.0);

// Returns true and sets intersection position and normal, or returns false
bool planeRay(Ray ray, Plane plane, out IntersectionInfo info) {
	float denom = dot(plane.normal, ray.dir); 

    vec3 p0l0 = plane.origin - ray.pos; 

    info.t = dot(p0l0, plane.normal) / denom; 
	info.pos = ray.pos + ray.dir * info.t;
	info.normal = -sign(denom) * plane.normal;
	
    return info.t >= 0; 
}

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(Ray ray, Sphere sphere, out IntersectionInfo info) {
	float R2 = sphere.radius * sphere.radius;
    vec3 L = sphere.center - ray.pos;
    float tca = dot(L, ray.dir);

    float D2 = dot(L, L) - tca * tca;
    if(D2 > R2) return false;

    float thc = sqrt(R2 - D2);
    float t0 = tca - thc;
    float t1 = tca + thc;
	
	info.t = min(t0, t1);
    info.pos = info.t * ray.dir + ray.pos;
	info.normal = normalize(info.pos - sphere.center);

    return D2 <= R2 && info.t >= 0;
}

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRay(Ray ray, Box box, out IntersectionInfo info) {
	float tmin = 0.0, tmax = 1.0 / 0.0;
	vec3 dirInv = vec3(1.0) / ray.dir;

    for (int i = 0; i < 3; ++i) {
        float t1 = (box.min[i] - ray.pos[i]) * dirInv[i];
        float t2 = (box.max[i] - ray.pos[i]) * dirInv[i];

        tmin = max(tmin, min(t1, t2));
        tmax = min(tmax, max(t1, t2));
    }

	info.t = tmin;
	info.pos = tmin * ray.dir + ray.pos;

	vec3 center = (box.max + box.min) * 0.5;
	vec3 sz = (box.max - box.min) * 0.5;
	float bias = 100000.0;

	info.normal = normalize(floor((info.pos - center) / abs(sz) * bias));

    return tmin < tmax;
}

vec3 getRayDir() {
	vec2 factor = vec2(gl_GlobalInvocationID.xy) / vec2(gl_NumWorkGroups.xy);

	vec3 top = mix(frustumTL, frustumTR, factor.x);
	vec3 bottom = mix(frustumBL, frustumBR, factor.x);

	return mix(bottom, top, factor.y);
}

void findClosest(
	bool hit, IntersectionInfo info, inout bool anyHit, inout IntersectionInfo closest
) {
	anyHit = anyHit || hit;

	int mixVal = int(hit && info.t < closest.t);
	closest.pos = mix(closest.pos, info.pos, mixVal);
	closest.normal = mix(closest.normal, info.normal, mixVal);
	closest.t = mix(closest.t, info.t, mixVal);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	Sphere s;
	s.center = vec3(-3.0, 3.0, 10.0);
	s.radius = 1.0;

	Box b;
	b.min = vec3(1.0, 1.0, 9.0);
	b.max = b.min + vec3(1.0);

	Plane p;
	p.origin = vec3(0.0);
	p.normal = vec3(0.0, 1.0, 0.0);

	Ray r;
	r.pos = frustumOrigin;
	r.dir = normalize(getRayDir());

	IntersectionInfo closest;
	closest.pos = vec3(0.0);
	closest.normal = vec3(0.0);
	closest.t = 10000000.0;

	IntersectionInfo current;
	current.pos = vec3(0.0);
	current.normal = vec3(0.0);
	current.t = 10000000.0;

	bool anyHit = false;

	findClosest(planeRay(r, p, current), current, anyHit, closest);
	findClosest(boxRay(r, b, current), current, anyHit, closest);
	findClosest(sphereRay(r, s, current), current, anyHit, closest);

	float lightFactor = max(0.0, dot(closest.normal, normalize(lightPos - closest.pos)));

	bool anyShadowHit = false;

	Ray sr;
	sr.pos = closest.pos + closest.normal * 0.001;
	sr.dir = normalize(lightPos - sr.pos);

	closest.pos = vec3(0.0);
	closest.normal = vec3(0.0);
	closest.t = 10000000.0;

	findClosest(planeRay(sr, p, current), current, anyShadowHit, closest);
	findClosest(boxRay(sr, b, current), current, anyShadowHit, closest);
	findClosest(sphereRay(sr, s, current), current, anyShadowHit, closest);

	vec3 surfaceColor = vec3(0.1) + vec3(1.0) * lightFactor * float(!anyShadowHit);

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