#include "/random"
#include "/triscene"

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
layout(location = 11) uniform vec3 lightPos = vec3(0, 4, 0);
layout(location = 12) uniform vec3 lightColor = vec3(1.5, 1.5, 1.5);
layout(location = 13) uniform int triCount = 0;

layout(std430, binding = 1) buffer TriangleInputs {
	Triangle triangles[];
};

layout(std430, binding = 2) buffer Materials {
	Material materials[];
};

bool traceScene(Ray r, out IntersectionInfo closest) {
	bool anyHit = false;

	TriIntersection closestInfo;
	TriIntersection interInfo;
	int closestTri = -1;

	closestInfo.t = 1000000;

	for (int i = 0; i < triCount; ++i) {
		Triangle tri = triangles[i];

		if (triangleRay(r, tri, interInfo)) {
			if (interInfo.t < closestInfo.t) {
				closestInfo = interInfo;
				closestTri = i;
			}
			anyHit = true;
		}
	}

	Triangle tri = triangles[closestTri];
	Material mat = materials[int(tri.materialId.x)];

	if (anyHit) {
		closest.t = closestInfo.t;
		closest.pos = closestInfo.pos;
		closest.normal =
			closestInfo.bary.x * tri.normal0uv1y.xyz + 
			closestInfo.bary.y * tri.normal1uv2x.xyz + 
			closestInfo.bary.z * tri.normal2uv2y.xyz;
		closest.color = mat.colorRoughness.xyz;
		closest.roughness = mat.colorRoughness.w;
	} else {
		closest.t = 1000000;
		closest.pos = vec3(0);
		closest.normal = vec3(0);
		closest.color = vec3(0);
		closest.roughness = 1;
	}

	return anyHit;
}

vec3 getRayDir() {
	vec2 jitter = vec2(sin(random * 6.29), cos(random * 6.29)) * jitter;
	vec2 factor = (vec2(gl_GlobalInvocationID.xy) + jitter) / vec2(gl_NumWorkGroups.xy);

	vec3 top = mix(frustumTL, frustumTR, factor.x);
	vec3 bottom = mix(frustumBL, frustumBR, factor.x);

	return mix(bottom, top, factor.y);
}

vec3 getDirectRadiance(Ray r, out IntersectionInfo rayInfo) {
	bool anyHit = traceScene(r, rayInfo);
	float lightFactor = max(0.0, dot(rayInfo.normal, normalize(lightPos - rayInfo.pos))) / 3.14159265;
	vec3 color = rayInfo.color;

	IntersectionInfo srInfo;

	Ray sr;
	sr.pos = rayInfo.pos + rayInfo.normal * 0.001;

	float shadow = 0.0;
	for(int i = 0; i < shadowSamples; ++i) {
		vec3 sampl = sampleSphere(lightPos, lightRadius, vec2(i + random) + r.dir.xy);

		sr.dir = normalize(sampl - sr.pos);
		traceScene(sr, srInfo);
		shadow += float(length(sampl - sr.pos) < srInfo.t);
	}

	shadow /= float(shadowSamples);

	return mix(vec3(0.0), color, float(anyHit)) * lightColor * lightFactor * shadow;
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