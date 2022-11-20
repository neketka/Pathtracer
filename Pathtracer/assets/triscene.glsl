#ifndef _TRISCENE_GLSL
#define _TRISCENE_GLSL

struct Ray {
  vec3 pos;
  vec3 dir;
  vec3 dirInv;
  float start;
  float end;
};

struct Triangle {
	vec4 pos0normx;
	vec4 pos1normy;
	vec4 pos2normz;
	uvec4 uvMatId;
};

struct Material {
  vec4 colorRoughness;
};

struct Box {
  vec3 min;
  vec3 max;
};

struct TriIntersection {
  vec3 pos;
  vec3 bary;
  float t;
};

struct IntersectionInfo {
  vec3 pos;
  vec3 normal;
  vec3 color;
  float roughness;
  float t;
};

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRay(Ray ray, Box box) {
  float tmin = ray.start, tmax = ray.end;

  for(int i = 0; i < 3; ++i) {
    float t1 = (box.min[i] - ray.pos[i]) * ray.dirInv[i];
    float t2 = (box.max[i] - ray.pos[i]) * ray.dirInv[i];

    tmin = max(tmin, min(t1, t2));
    tmax = min(tmax, max(t1, t2));
  }

  return tmin < tmax;
}

bool triangleRay(Ray ray, Triangle tri, inout TriIntersection info) {
  const float EPSILON = 0.0000001;

  vec3 v0 = tri.pos0normx.xyz;
  vec3 v1 = tri.pos1normy.xyz;
  vec3 v2 = tri.pos2normz.xyz;

  vec3 e1 = v1 - v0;
  vec3 e2 = v2 - v0;

  vec3 h = cross(ray.dir, e2);
  float a = dot(e1, h);
  if (a > -EPSILON && a < EPSILON) 
    return false;
  
  float f = 1.0 / a;
  vec3 s = ray.pos - v0;
  float u = f * dot(s, h);
  if (u < 0.0 || u > 1.0)
    return false;

  vec3 q = cross(s, e1);
  float v = f * dot(ray.dir, q);

  if (v < 0.0 || u + v > 1.0)
    return false;

  float t = f * dot(e2, q);

  if (t < ray.start || t > ray.end)
    return false;

  info.pos = ray.pos + ray.dir * t;
  info.bary = vec3(u, v, 1 - u - v);
  info.t = t;

  return t > EPSILON;
}

#endif