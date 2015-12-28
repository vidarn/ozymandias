#pragma once
#include <math.h>
#include <stdio.h>
#include "math_common.h"
#include "common.h"
#include "vec3.h"

static inline CONST
float dot_vec4(Vec4 a, Vec4 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static inline CONST
Vec4 normalize_vec4(Vec4 a)
{
    float inv_magnitude = 1.f/sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
    a.x *= inv_magnitude;
    a.y *= inv_magnitude;
    a.z *= inv_magnitude;
    a.w *= inv_magnitude;
    return a;
}

static inline CONST
float magnitude_vec4(Vec4 a)
{
    return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
}

static inline CONST
float magnitude_sq_vec4(Vec4 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w;
}


static inline CONST
Vec4 scale_vec4(Vec4 a, float f)
{
    return vec4(f*a.x,f*a.y,f*a.z,f*a.w);
}

/*static inline OVERLOADABLE
void print(Vec4 a){
    printf("[%f, %f, %f, %f]^T\n",a.x,a.y,a.z,a.w);
}*/

