#include "/triscene"

struct BvhNode {
	vec4 minExtent;
	vec4 maxExtent;
	ivec4 navigation;
	// left, right, parent, tri
};

// https://tavianator.com/2022/ray_box_boundary.html
float nodeRay(Ray ray, BvhNode box) {
  vec3 tMin = (box.minExtent.xyz - ray.pos) * ray.dirInv;
  vec3 tMax = (box.maxExtent.xyz - ray.pos) * ray.dirInv;

  vec3 t1 = min(tMin, tMax);
  vec3 t2 = max(tMin, tMax);

  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);

  if (tNear < tFar && tNear < ray.end && tFar > ray.start) {
    return max(tNear, ray.start);
  }
  
  return -1;
}