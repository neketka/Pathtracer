#include "/triscene"

struct BvhNode {
	vec4 minExtent;
	vec4 maxExtent;
	ivec4 navigation;
	// left, right, parent, tri
};

// https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
float nodeRay(Ray ray, BvhNode box) {
  vec3 tMin = (box.minExtent.xyz + vec3(0.001) - ray.pos) * ray.dirInv;
  vec3 tMax = (box.maxExtent.xyz - vec3(0.001) - ray.pos) * ray.dirInv;

  vec3 t1 = min(tMin, tMax);
  vec3 t2 = max(tMin, tMax);

  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);

  if (tNear < tFar && tNear < ray.end && tFar > ray.start) {
    return max(tNear, ray.start);
  }
  
  return -1;
}