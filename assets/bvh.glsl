#include "/triscene"

struct BvhNode {
	vec4 minExtent;
	vec4 maxExtent;
	ivec4 navigation;
	// left, right, parent, tri
};

// https://tavianator.com/2022/ray_box_boundary.html
float nodeRay(Ray ray, BvhNode box) {

  return 0;

  float tmin = ray.start, tmax = ray.end;

  for(int i = 0; i < 3; ++i) {
    float t1 = (box.minExtent[i] - ray.pos[i]) * ray.dirInv[i];
    float t2 = (box.maxExtent[i] - ray.pos[i]) * ray.dirInv[i];

    tmin = max(tmin, min(t1, t2));
    tmax = min(tmax, max(t1, t2));
  }

  if (tmin < tmax) {
    return tmin;
  }
  
  return -1;
}