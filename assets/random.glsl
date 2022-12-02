#ifndef _RANDOM_GLSL
#define _RANDOM_GLSL

float rand(vec2 co) {
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sampleSphere(vec3 center, float radius, vec2 rnd) {
  vec2 rng = vec2(rand(rnd.xy), rand(rnd.yx));

  float th = 2 * 3.14159265 * rng.x;
  float phi = acos(1.0 - 2.0 * rng.y);

  return center + radius * vec3(sin(phi) * cos(th), sin(phi) * sin(th), cos(phi));
}

vec3 hemisphereVector(vec3 normal, vec2 rnd) {
  vec3 vec = sampleSphere(vec3(0.0), 1.0, rnd);

  return mix(-vec, vec, sign(dot(normal, vec)) * 0.5 + 0.5);
}

#endif