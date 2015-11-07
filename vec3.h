#pragma once
#include <math.h>
#include <stdio.h>
#include "math_common.h"

typedef float vec3 __attribute__((ext_vector_type(4)));

static inline
float dot(vec3 a, vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline
vec3 cross(vec3 a, vec3 b)
{
    vec3 ret;
    ret.x = a.y*b.z - a.z*b.y;
    ret.y = a.z*b.x - a.x*b.z;
    ret.z = a.x*b.y - a.y*b.x;
    return ret;
}

static inline
vec3 normalize(vec3 a)
{
    float inv_magnitude = 1.f/sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
    a.x *= inv_magnitude;
    a.y *= inv_magnitude;
    a.z *= inv_magnitude;
    return a;
}

static inline
float magnitude(vec3 a)
{
    return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

static inline
float magnitude_sq(vec3 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z;
}

static inline
float vec3_max_element(vec3 a)
{
    return max(a.x,max(a.y,a.z));
}

static inline
float vec3_min_element(vec3 a)
{
    return min(a.x,min(a.y,a.z));
}

static inline
vec3 scale_vec3(vec3 a, float f)
{
    return (vec3){f*a.x,f*a.y,f*a.z};
}


static inline
void __attribute__((overloadable)) print(vec3 a){
    printf("[%f, %f, %f]\n",a.x,a.y,a.z);
}

static inline
vec3 __attribute__((overloadable)) invert(vec3 a)
{
    return (vec3){-a.x,-a.y,-a.z};
}


