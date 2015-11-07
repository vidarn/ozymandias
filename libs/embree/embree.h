#pragma once
#include "../../vec3.h"
#include "../../scene.h"

#ifdef _WIN32
#  define EMBREE_ALIGN(a) __declspec(align(a))
#else
#  define EMBREE_ALIGN(a) __attribute__((aligned(a)))
#endif

typedef struct EMBREE_ALIGN(16) {
  /* ray data */
  vec3 org;      // Ray origin
  
  vec3  dir;      // Ray direction
  
  float tnear;       // Start of ray segment
  float tfar;        // End of ray segment (set to hit distance)

  float time;        // Time of this ray for motion blur
  int   mask;        // Used to mask out objects during traversal
  
  /* hit data */
  vec3  Ng;       // Unnormalized geometry normal

  float u;           // Barycentric u coordinate of hit
  float v;           // Barycentric v coordinate of hit

  int   geomID;        // geometry ID
  int   primID;        // primitive ID
  int   instID;        // instance ID
}Ray;

//TODO(Vidar): Finish this
typedef struct {} EmbreeScene; // Opaque struct wrapping RTCScene

#ifdef __cplusplus 
extern "C" {
#endif
    EmbreeScene *embree_init(Scene ozy_scene);
    void embree_set_ray(Ray *ray, vec3 org, vec3 dir, float tnear, float tfar);
    char embree_occluded (Ray *ray, EmbreeScene *scene);
    char embree_intersect(Ray *ray, EmbreeScene *scene);
    void embree_close(EmbreeScene *scene);
#ifdef __cplusplus 
}
#endif
