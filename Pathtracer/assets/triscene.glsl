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
  vec4 colorRM;
};

struct TriIntersection {
  vec3 pos;
  vec3 normal;
  vec3 bary;
  float t;
};

struct IntersectionInfo {
  vec3 pos;
  vec3 normal;
  vec3 color;
  vec3 bary;
  float metalness;
  float roughness;
  float t;
  int triIndex;
  bool anyHit;
};

bool triangleRay(Ray ray, Triangle tri, inout TriIntersection info) {
  const float EPSILON = 0.000001;

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
  info.normal = normalize(cross(e1, e2));
  info.normal *= -sign(dot(info.normal, ray.dir));
  info.bary = vec3(u, v, 1 - u - v);
  info.t = t;

  return true;
}

#endif