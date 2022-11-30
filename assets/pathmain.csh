#include "/random"
#include "/bvh"
#include "/triscene"

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D target;

layout(location = 0) uniform mat4 viewBasis = mat4(1.0);
layout(location = 1) uniform int samples;
layout(location = 2) uniform float random;
layout(location = 3) uniform float jitter;
layout(location = 4) uniform int bounces;
layout(location = 5) uniform float lightRadius;
layout(location = 6) uniform vec3 lightPos = vec3(0, 4, 0);
layout(location = 7) uniform vec3 lightColor = vec3(10.0, 10.0, 10.0);

layout(std430, binding = 0) buffer TriangleInputs {
	Triangle triangles[];
};

layout(std430, binding = 1) buffer BvhNodes {
	BvhNode bvhNodes[];
};

layout(std140, binding = 0) uniform Materials {
	Material materials[64];
};

bool traceScene(Ray r, bool anyReturn, out IntersectionInfo closest) {
	bool anyHit = false;
	closest.anyHit = false;

	TriIntersection closestInfo;
	int closestTri = -1;

	int depth = 1;
	int nodeIndex = 0;
	int next[32];

	next[0] = -1;
	next[1] = 0;

	// https://developer.nvidia.com/blog/thinking-parallel-part-ii-tree-traversal-gpu/
	do {
		BvhNode node = bvhNodes[nodeIndex];
		BvhNode left = bvhNodes[node.navigation.x];
		BvhNode right = bvhNodes[node.navigation.y];

		float leftInter = nodeRay(r, bvhNodes[node.navigation.x]);
		float rightInter = nodeRay(r, bvhNodes[node.navigation.y]);

		if (leftInter != -1 && left.navigation.w != -1) {
			Triangle tri = triangles[left.navigation.w];

			bool hit = triangleRay(r, tri, closestInfo);
			if (anyReturn && hit) {
				return true;
			} else if (hit) {
				r.end = closestInfo.t;
				closestTri = left.navigation.w;
				anyHit = true;
			}
		}

		if (rightInter != -1 && right.navigation.w != -1) {
			Triangle tri = triangles[right.navigation.w];

			bool hit = triangleRay(r, tri, closestInfo);
			if (anyReturn && hit) {
				return true;
			} else if (hit) {
				r.end = closestInfo.t;
				closestTri = right.navigation.w;
				anyHit = true;
			}
		}
		
		bool traverseL = (leftInter != -1 && left.navigation.w == -1);
		bool traverseR = (rightInter != -1 && right.navigation.w == -1);
		
		if (!traverseL && !traverseR) {
			nodeIndex = next[--depth];
		} else {
			if (traverseL && traverseR) {
				if (leftInter < rightInter) {
					nodeIndex = node.navigation.x;
					next[depth++] = node.navigation.y;
				} else {
					nodeIndex = node.navigation.y;
					next[depth++] = node.navigation.x;
				}
			} else {
				nodeIndex = traverseL ? node.navigation.x : node.navigation.y;
			}
		}
	} while (nodeIndex != -1);

	Triangle tri = triangles[closestTri];
	Material mat = materials[tri.uvMatId.w];

	if (anyHit) {
		closest.t = closestInfo.t;
		closest.pos = closestInfo.pos;
		closest.normal = vec3(tri.pos0normx.w, tri.pos1normy.w, tri.pos2normz.w);
		closest.color = mat.colorRoughness.xyz;
		closest.roughness = mat.colorRoughness.w;
		closest.anyHit = true;
	}

	return anyHit;
}

vec3 getRayDir() {
	vec2 jitter = vec2(sin(random * 6.29), cos(random * 6.29)) * jitter;
	vec2 factor = (vec2(gl_GlobalInvocationID.xy) + vec2(0.5) + jitter) / vec2(gl_NumWorkGroups.xy);

	return (viewBasis * vec4(factor * 2.0 - vec2(1.0), 1.0, 0.0)).xyz;
}

vec3 getDirectRadiance(Ray r, out IntersectionInfo rayInfo) {
	bool anyHit = traceScene(r, false, rayInfo);

	if (anyHit) {
		vec3 toLight = lightPos - rayInfo.pos;
		float lightDistRecip = 1.0 / length(toLight);

		float lightFactor = max(0.0, dot(rayInfo.normal, toLight * lightDistRecip)) * lightDistRecip * lightDistRecip;
		vec3 color = rayInfo.color;
		IntersectionInfo srInfo;

		vec3 sampl = sampleSphere(lightPos, lightRadius, vec2(random) + r.dir.xy);
		vec3 toSampl = sampl - rayInfo.pos;
		float dist = length(toSampl);

		Ray sr;
		sr.pos = rayInfo.pos;
		sr.start = 0.000001;
		sr.dir = (sampl - sr.pos) / dist;
		sr.dirInv = 1.0 / sr.dir;
		sr.end = dist;

		float shadow = float(!traceScene(sr, true, srInfo));

		return color * lightColor * lightFactor * shadow;
	}

	return vec3(0);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	Ray r;
	r.pos = viewBasis[3].xyz;
	r.dir = getRayDir();
	r.dirInv = vec3(1.0) / r.dir;
	r.start = 0.0;
	r.end = 1000000.0;

	IntersectionInfo rayInfo;

	vec3 directLight = getDirectRadiance(r, rayInfo);
	vec3 indirectLight = vec3(0.0);

	vec3 curColor = vec3(1.0);

	for (int i = 0; i < bounces; ++i) {
		if (!rayInfo.anyHit) {
			break;
		}

		curColor *= rayInfo.color;

		vec3 curNormal = rayInfo.normal;
		float curRoughness = rayInfo.roughness;

		r.start = 0.000001;
		r.end = 1000000.0;
		r.pos = rayInfo.pos;
		r.dir = mix(reflect(r.dir, curNormal), hemisphereVector(curNormal, vec2(random) + r.dir.xy), curRoughness);
		r.dirInv = 1.0 / r.dir;

		vec3 rad = getDirectRadiance(r, rayInfo);
		if (rad == vec3(0.0)) {
			break;
		}

		float lightFactor = max(0.0, abs(dot(curNormal, normalize(rayInfo.pos - r.pos))));
		indirectLight += rad * curColor * mix(1.0, lightFactor, curRoughness);
	}

	vec3 pixel = clamp(directLight + indirectLight, vec3(0.0), vec3(1.0));
	vec4 color = imageLoad(target, pos);
	imageStore(target, pos, (color * samples + vec4(pixel, 1.0)) / (samples + 1));
}