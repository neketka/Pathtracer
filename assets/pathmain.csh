#include "/primitives"
#include "/random"

void findClosest(bool hit, IntersectionInfo info, inout bool anyHit, inout IntersectionInfo closest) {
	anyHit = anyHit || hit;
	float mixVal = 1.0;

	if(hit && (info.t < closest.t)) {
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

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D target;

layout(location = 0) uniform vec3 frustumTL = vec3(-0.577, 0.577, 0.577);
layout(location = 1) uniform vec3 frustumTR = vec3(0.577, 0.577, 0.577);
layout(location = 2) uniform vec3 frustumBL = vec3(-0.577, -0.577, 0.577);
layout(location = 3) uniform vec3 frustumBR = vec3(0.577, -0.577, 0.577);
layout(location = 4) uniform vec3 frustumOrigin = vec3(0.0);
layout(location = 5) uniform int samples;
layout(location = 6) uniform float random;
layout(location = 7) uniform float jitter;
layout(location = 8) uniform int bounces;
layout(location = 9) uniform int shadowSamples;
layout(location = 10) uniform float lightRadius;

const vec3 lightPos = vec3(0.0, 4.0, 0.0);

vec3 getRayDir() {
	vec2 jitter = vec2(sin(random * 6.29), cos(random * 6.29)) * jitter;
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
	for(int i = 0; i < shadowSamples; ++i) {
		vec3 sampl = sampleSphere(lightPos, lightRadius, vec2(i + random) + r.dir.xy);

		sr.dir = normalize(sampl - sr.pos);
		traceScene(sr, srInfo);
		shadow += float(length(sampl - sr.pos) <= srInfo.t);
	}

	shadow /= float(shadowSamples);

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

	for(int i = 0; i < bounces; ++i) {
		vec3 curNormal = rayInfo.normal;
		vec3 curColor = rayInfo.color;
		float curRoughness = rayInfo.roughness;

		r.pos = rayInfo.pos + rayInfo.normal * 0.001;
		r.dir = mix(reflect(r.dir, curNormal), hemisphereVector(curNormal, vec2(random) + r.dir.xy), curRoughness);

		vec3 rad = getDirectRadiance(r, rayInfo);

		float lightFactor = max(0.0, abs(dot(curNormal, normalize(rayInfo.pos - r.pos)))) * brdf;
		indirectLight += rad * curColor * mix(1.0, lightFactor, curRoughness);

		brdf = mix(brdf, brdf * brdfFactor, curRoughness);
	}

	vec3 pixel = clamp(directLight + indirectLight, vec3(0.0), vec3(1.0));
	vec4 color = imageLoad(target, pos);
	imageStore(target, pos, (color * samples + vec4(pixel, 1.0)) / (samples + 1));
}