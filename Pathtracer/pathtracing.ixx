module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <random>

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
	vec3 color;
	float roughness;
	float t;
};

struct Sphere {
	vec3 center;
	float radius;
	vec3 color;
	float roughness;
};

struct Plane {
	vec3 origin;
	vec3 normal;
};

struct Box {
	vec3 min;
	vec3 max;
};

struct Triangle {
	vec3 pos0;
	vec3 pos1;
	vec3 pos2;
	vec3 color;
	float roughness;
};

// Returns true and sets intersection position and normal, or returns false
bool planeRay(Ray ray, Plane plane, inout IntersectionInfo info) {
	float denom = dot(plane.normal, ray.dir); 

    vec3 p0l0 = plane.origin - ray.pos; 

    info.t = dot(p0l0, plane.normal) / denom; 
	info.pos = ray.pos + ray.dir * info.t;
	info.normal = -sign(denom) * plane.normal;
	
    return info.t >= 0; 
}

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(Ray ray, Sphere sphere, inout IntersectionInfo info) {
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
	info.color = sphere.color;
	info.roughness = sphere.roughness;

    return D2 <= R2 && info.t >= 0;
}

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRay(Ray ray, Box box, inout IntersectionInfo info) {
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

bool triangleRay(Ray ray, Triangle tri, inout IntersectionInfo info){
	// compute plane's normal
    vec3 v0v1 = tri.pos1 - tri.pos0; 
    vec3 v0v2 = tri.pos2 - tri.pos0; 
    // no need to normalize
    vec3 N = cross(v0v1, v0v2);  //N 
    float area2 = length(N); 
 
    // Step 1: finding P
 
    // check if ray and plane are parallel.
    float NdotRayDirection = dot(N, ray.dir); 
    if (abs(NdotRayDirection) < 0.000001)  //almost 0 
        return false;  //they are parallel so they don't intersect ! 
 
    // compute d parameter using equation 2
    float d = -dot(N,tri.pos0); 
 
    // compute t (equation 3)
    info.t = -(dot(N,ray.pos) + d) / NdotRayDirection; 
 
    // check if the triangle is in behind the ray
    if (info.t < 0) return false;  //the triangle is behind 
 
    // compute the intersection point using equation 1
    info.pos = ray.pos + info.t * ray.dir; 
 
    // Step 2: inside-outside test
    vec3 C;  //vector perpendicular to triangle's plane 
 
    // edge 0
    vec3 edge0 = tri.pos1 - tri.pos0; 
    vec3 vp0 = info.pos - tri.pos0; 
    C = cross(edge0, vp0); 
    if (dot(N, C) < 0) return false;  //P is on the right side 
 
    // edge 1
    vec3 edge1 = tri.pos2 - tri.pos1; 
    vec3 vp1 = info.pos - tri.pos1; 
    C = cross(edge1, vp1); 
    if (dot(N,C) < 0)  return false;  //P is on the right side 
 
    // edge 2
    vec3 edge2 = tri.pos0 - tri.pos2; 
    vec3 vp2 = info.pos - tri.pos2; 
    C = cross(edge2, vp2); 
    if (dot(N,C) < 0) return false;  //P is on the right side; 
	
	info.normal = -sign(NdotRayDirection) * normalize(N);
	info.color = tri.color;
	info.roughness = tri.roughness;

    return true;  //this ray hits the triangle 
}

void findClosest(
	bool hit, IntersectionInfo info, inout bool anyHit, inout IntersectionInfo closest
) {
	anyHit = anyHit || hit;
	float mixVal = 1.0;

	if (hit && (info.t < closest.t)) {
		closest.pos = info.pos;
		closest.normal = info.normal;
		closest.t = info.t;
		closest.color = info.color;
		closest.roughness = info.roughness;
	}
}

bool traceScene(Ray r, out IntersectionInfo closest) {
	Sphere s;
	s.center = vec3(-3.0, 3.0, 1.0);
	s.radius = 1.0;
	s.color = vec3(1.0);
	s.roughness = 0.0;

	Sphere s2;
	s2.center = vec3(3.0, 3.0, 1.0);
	s2.radius = 1.0;
	s2.color = vec3(1.0, 0.0, 1.0);
	s2.roughness = 1.0;

	Sphere s3;
	s3.center = vec3(-3.0, -3.0, 1.0);
	s3.radius = 3.0;
	s3.color = vec3(1.0);
	s3.roughness = 0.0;

	Plane left;
	left.origin = vec3(-6.0, -6.0, -6.0);
	left.normal = vec3(1.0, 0.0, 0.0);

	Plane right;
	right.origin = vec3(5.0, -6.0, -6.0);
	right.normal = vec3(-1.0, 0.0, 0.0);

	Plane top;
	top.origin = vec3(-6.0, 5.0, -6.0);
	top.normal = vec3(0.0, -1.0, 0.0);

	Plane bottom;
	bottom.origin = vec3(-6.0, -6.0, -6.0);
	bottom.normal = vec3(0.0, 1.0, 0.0);

	Plane front;
	front.origin = vec3(-6.0, -6.0, 5.0);
	front.normal = vec3(0.0, 0.0, -1.0);

	Plane back;
	back.origin = vec3(-6.0, -6.0, -6.0);
	back.normal = vec3(0.0, 0.0, 1.0);

	Triangle tri;
	tri.pos0 = vec3(-2.0, -2.0, -2.0);
	tri.pos1 = vec3(-2.0, 2.0, 2.0);
	tri.pos2 = vec3(2.0, 2.0, 2.0);
	tri.color = vec3(1.0, 1.0, 1.0);
	tri.roughness = 0.0;
	
	closest.pos = vec3(0.0);
	closest.normal = vec3(0.0);
	closest.t = 10000000.0;
	closest.color = vec3(1.0);
	closest.roughness = 1.0;

	IntersectionInfo current;
	current.pos = vec3(0.0);
	current.normal = vec3(0.0);
	current.t = 10000000.0;
	current.color = vec3(1.0);
	current.roughness = 1.0;

	bool anyHit = false;

	findClosest(planeRay(r, left, current), current, anyHit, closest);
	findClosest(planeRay(r, right, current), current, anyHit, closest);
	findClosest(planeRay(r, top, current), current, anyHit, closest);
	findClosest(planeRay(r, bottom, current), current, anyHit, closest);
	findClosest(planeRay(r, front, current), current, anyHit, closest);
	findClosest(planeRay(r, back, current), current, anyHit, closest);
	findClosest(sphereRay(r, s, current), current, anyHit, closest);
	findClosest(sphereRay(r, s2, current), current, anyHit, closest);
	findClosest(sphereRay(r, s3, current), current, anyHit, closest);
	findClosest(triangleRay(r, tri, current), current, anyHit, closest);

	return anyHit;
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sampleSphere(vec3 center, float radius, vec2 rnd) {
	vec2 rng = vec2(rand(rnd), rand(rnd.yx));

	float th = 2 * 3.14159265 * rng.x;
	float phi = acos(1.0 - 2.0 * rng.y);

	return center + radius * vec3(
		sin(phi)*cos(th), sin(phi)*sin(th), cos(phi)
	);
}

vec3 hemisphereVector(vec3 normal, vec2 rnd) {
	vec3 vec = sampleSphere(vec3(0.0), 1.0, rnd);

	return mix(-vec, vec, sign(dot(normal, vec)) * 0.5 + 0.5);
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D target;

layout(location = 0) uniform vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
layout(location = 1) uniform vec3 frustumTR = vec3(0.577, 0.577, 0.577);
layout(location = 2) uniform vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
layout(location = 3) uniform vec3 frustumBR = vec3(0.577, -0.577, 0.577);
layout(location = 4) uniform vec3 frustumOrigin = vec3(0.0);
layout(location = 5) uniform int samples;
layout(location = 6) uniform float time;

const vec3 lightPos = vec3(0.0, 4.0, 0.0);
const float lightRadius = 1.0;

vec3 getRayDir() {
	vec2 jitter = vec2(sin(time), cos(time)) * 2.0;
	vec2 factor = (vec2(gl_GlobalInvocationID.xy) + jitter) / vec2(gl_NumWorkGroups.xy);

	vec3 top = mix(frustumTL, frustumTR, factor.x);
	vec3 bottom = mix(frustumBL, frustumBR, factor.x);

	return mix(bottom, top, factor.y);
}

vec3 getDirectRadiance(Ray r, out IntersectionInfo rayInfo) {
	bool anyHit = traceScene(r, rayInfo);
	float lightFactor = max(0.0, dot(rayInfo.normal, normalize(lightPos - rayInfo.pos))) / 3.14159265 * 1.3;
	vec3 color = rayInfo.color;

	IntersectionInfo srInfo;

	Ray sr;
	sr.pos = rayInfo.pos + rayInfo.normal * 0.001;

	float shadow = 0.0;
	for (int i = 0; i < 2; ++i) {
		vec3 sampl = sampleSphere(lightPos, lightRadius, vec2(i + time) + r.dir.xy);
		
		sr.dir = normalize(sampl - sr.pos);
		traceScene(sr, srInfo);
		shadow += float(length(sampl - sr.pos) <= srInfo.t);
	}

	shadow /= 2.0;

	return mix(vec3(0.0), color, float(anyHit)) * lightFactor * shadow;
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	Ray r;
	r.pos = frustumOrigin;
	r.dir = normalize(getRayDir());
	
	IntersectionInfo rayInfo;
	
	vec3 directLight = getDirectRadiance(r, rayInfo);
	vec3 indirectLight = vec3(0.0);

	const float brdfFactor = 1.0 / 3.14159265;
	float brdf = 1.0;

	for (int i = 0; i < 3; ++i) {
		vec3 curNormal = rayInfo.normal;
		vec3 curColor = rayInfo.color;
		float curRoughness = rayInfo.roughness;
		
		r.pos = rayInfo.pos + rayInfo.normal * 0.001;
		r.dir = mix(reflect(r.dir, curNormal), hemisphereVector(curNormal, vec2(time) + r.dir.xy), curRoughness);

		vec3 rad = getDirectRadiance(r, rayInfo);

		float lightFactor = max(0.0, abs(dot(curNormal, normalize(rayInfo.pos - r.pos)))) * brdf;
		indirectLight += rad * curColor * mix(1.0, lightFactor, curRoughness);

		brdf = mix(brdf, brdf * brdfFactor, curRoughness);
	}

	vec3 pixel = clamp(directLight + indirectLight, vec3(0.0), vec3(1.0));
	vec4 color = imageLoad(target, pos);
	imageStore(target, pos, (color * samples + vec4(pixel, 1.0)) / (samples + 1));
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
	PathtracingPass() : m_dist(0.0, 1.0) {
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

		if (mvp != m_mvp) {
			m_samples = 0;
			m_mvp = mvp;
		}

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
					.uniform(5, m_samples++)
					.uniform(6, m_dist(m_gen))
			)
		);

		pass.attach();
		pass.dispatch(target->w(), target->h(), 1);
	}
private:
	std::uniform_real_distribution<float> m_dist;
	std::default_random_engine m_gen;

	glm::mat4 m_mvp;
	int m_samples = 0;
	GpuProgram* m_program;
};