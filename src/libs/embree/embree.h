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
  Vec3 org;      // Ray origin
  
  Vec3  dir;      // Ray direction
  
  float tnear;       // Start of ray segment
  float tfar;        // End of ray segment (set to hit distance)

  float time;        // Time of this ray for motion blur
  int   mask;        // Used to mask out objects during traversal
  
  /* hit data */
  Vec3  Ng;       // Unnormalized geometry normal

  float u;           // Barycentric u coordinate of hit
  float v;           // Barycentric v coordinate of hit

  u32   geomID;        // geometry ID
  u32   primID;        // primitive ID
  u32   instID;        // instance ID
}Ray;

//TODO(Vidar): "It is strongly recommended to have the Flush to Zero and Denormals are Zero mode of the MXCSR control and status register enabled for each thread before calling the rtcIntersect and rtcOccluded functions. Otherwise, under some circumstances special handling of denormalized floating point numbers can significantly reduce application and Embree performance."

typedef struct EmbreeScene EmbreeScene; // Opaque struct wrapping RTCScene

#ifdef __cplusplus 
extern "C" {
#endif
    EmbreeScene *embree_init(OzyScene ozy_scene);
    void embree_set_ray(Ray *ray, Vec3 org, Vec3 dir, float tnear, float tfar);
    char embree_occluded (Ray *ray, EmbreeScene *scene);
    char embree_intersect(Ray *ray, EmbreeScene *scene);
    void embree_close(EmbreeScene *scene);
#ifdef __cplusplus 
}
#endif
