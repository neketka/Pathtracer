#ifndef _TRISCENE_GLSL
#define _TRISCENE_GLSL

struct Ray {
  vec3 pos;
  vec3 dir;
};

struct Triangle {
  vec4 pos0uv0x;
  vec4 pos1uv0y;
  vec4 pos2uv1x;

  vec4 normal0uv1y;
  vec4 normal1uv2x;
  vec4 normal2uv2y;

  vec4 materialId;
  vec4 padding0;
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
  float tmin = 0.0, tmax = 1.0 / 0.0;
  vec3 dirInv = vec3(1.0) / ray.dir;

  for(int i = 0; i < 3; ++i) {
    float t1 = (box.min[i] - ray.pos[i]) * dirInv[i];
    float t2 = (box.max[i] - ray.pos[i]) * dirInv[i];

    tmin = max(tmin, min(t1, t2));
    tmax = min(tmax, max(t1, t2));
  }

  return tmin < tmax;
}

bool triangleRay(Ray ray, Triangle tri, inout TriIntersection info) {
  const float EPSILON = 0.0000001;

  vec3 v0 = tri.pos0uv0x.xyz;
  vec3 v1 = tri.pos1uv0y.xyz;
  vec3 v2 = tri.pos2uv1x.xyz;

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

  info.pos = ray.pos + ray.dir * t;
  info.bary = vec3(u, v, 1 - u - v);
  info.t = t;

  return t > EPSILON;
}

#endif