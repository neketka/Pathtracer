#ifndef _PRIMITIVES_GLSL
#define _PRIMITIVES_GLSL

struct Ray {
  vec3 pos;
  vec3 dir;
};

struct IntersectionInfo {
  vec3 pos;
  vec3 normal;
  vec3 color;
  float roughness;
  float t;
};

struct Sphere {
  vec3 center;
  float radius;
  vec3 color;
  float roughness;
};

struct Plane {
  vec3 origin;
  vec3 normal;
};

struct Box {
  vec3 min;
  vec3 max;
};

struct Triangle {
  vec3 pos0;
  vec3 pos1;
  vec3 pos2;
  vec3 color;
  float roughness;
};

// Returns true and sets intersection position and normal, or returns false
bool planeRay(Ray ray, Plane plane, inout IntersectionInfo info) {
  float denom = dot(plane.normal, ray.dir);

  vec3 p0l0 = plane.origin - ray.pos;

  info.t = dot(p0l0, plane.normal) / denom;
  info.pos = ray.pos + ray.dir * info.t;
  info.normal = -sign(denom) * plane.normal;

  return info.t >= 0;
}

// Returns true and sets intersection position and normal, or returns false
bool sphereRay(Ray ray, Sphere sphere, inout IntersectionInfo info) {
  float R2 = sphere.radius * sphere.radius;
  vec3 L = sphere.center - ray.pos;
  float tca = dot(L, ray.dir);

  float D2 = dot(L, L) - tca * tca;
  if(D2 > R2)
    return false;

  float thc = sqrt(R2 - D2);
  float t0 = tca - thc;
  float t1 = tca + thc;

  info.t = min(t0, t1);
  info.pos = info.t * ray.dir + ray.pos;
  info.normal = normalize(info.pos - sphere.center);
  info.color = sphere.color;
  info.roughness = sphere.roughness;

  return D2 <= R2 && info.t >= 0;
}

// https://tavianator.com/2022/ray_box_boundary.html
bool boxRaySimple(Ray ray, Box box) {
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

bool triangleRay(Ray ray, Triangle tri, inout IntersectionInfo info) {
	// compute plane's normal
  vec3 v0v1 = tri.pos1 - tri.pos0;
  vec3 v0v2 = tri.pos2 - tri.pos0; 
    // no need to normalize
  vec3 N = cross(v0v1, v0v2);  //N 
  float area2 = length(N); 

    // Step 1: finding P

    // check if ray and plane are parallel.
  float NdotRayDirection = dot(N, ray.dir);
  if(abs(NdotRayDirection) < 0.000001)  //almost 0 
    return false;  //they are parallel so they don't intersect ! 

    // compute d parameter using equation 2
  float d = -dot(N, tri.pos0); 

    // compute t (equation 3)
  info.t = -(dot(N, ray.pos) + d) / NdotRayDirection; 

    // check if the triangle is in behind the ray
  if(info.t < 0)
    return false;  //the triangle is behind 

    // compute the intersection point using equation 1
  info.pos = ray.pos + info.t * ray.dir; 

    // Step 2: inside-outside test
  vec3 C;  //vector perpendicular to triangle's plane 

    // edge 0
  vec3 edge0 = tri.pos1 - tri.pos0;
  vec3 vp0 = info.pos - tri.pos0;
  C = cross(edge0, vp0);
  if(dot(N, C) < 0)
    return false;  //P is on the right side 

    // edge 1
  vec3 edge1 = tri.pos2 - tri.pos1;
  vec3 vp1 = info.pos - tri.pos1;
  C = cross(edge1, vp1);
  if(dot(N, C) < 0)
    return false;  //P is on the right side 

    // edge 2
  vec3 edge2 = tri.pos0 - tri.pos2;
  vec3 vp2 = info.pos - tri.pos2;
  C = cross(edge2, vp2);
  if(dot(N, C) < 0)
    return false;  //P is on the right side; 

  info.normal = -sign(NdotRayDirection) * normalize(N);
  info.color = tri.color;
  info.roughness = tri.roughness;

  return true;  //this ray hits the triangle 
}

#endif