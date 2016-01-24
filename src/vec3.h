#pragma once
#include <math.h>
#include <stdio.h>
#include "math_common.h"
#include "common.h"

typedef union{
    float v[4];
    struct {
        float x,y,z,w;
    };
}Vec4;

typedef Vec4 Vec3;

static inline CONST
Vec3 vec3(float x, float y, float z)
{
    Vec3 v = {{x,y,z,0.f}};
    return v;
}
static inline CONST
Vec4 vec4(float x, float y, float z, float w)
{
    Vec4 v = {{x,y,z,w}};
    return v;
}

static inline CONST
float dot(Vec3 a, Vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline CONST
Vec3 cross(Vec3 a, Vec3 b)
{
    Vec3 ret;
    ret.x = a.y*b.z - a.z*b.y;
    ret.y = a.z*b.x - a.x*b.z;
    ret.z = a.x*b.y - a.y*b.x;
    return ret;
}

static inline CONST
Vec3 add_vec3(Vec3 a, Vec3 b)
{
    Vec3 ret = {{a.x+b.x, a.y+b.y, a.z+b.z}};
    return ret;
}

static inline CONST
Vec3 sub_vec3(Vec3 a, Vec3 b)
{
    Vec3 ret = {{a.x-b.x, a.y-b.y, a.z-b.z}};
    return ret;
}

static inline CONST
Vec3 mul_vec3(Vec3 a, Vec3 b)
{
    Vec3 ret = {{a.x*b.x, a.y*b.y, a.z*b.z}};
    return ret;
}

static inline CONST
Vec3 normalize(Vec3 a)
{
    float inv_magnitude = 1.f/sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
    a.x *= inv_magnitude;
    a.y *= inv_magnitude;
    a.z *= inv_magnitude;
    return a;
}

static inline CONST
float magnitude(Vec3 a)
{
    return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

static inline CONST
float magnitude_sq(Vec3 a)
{
    return a.x*a.x + a.y*a.y + a.z*a.z;
}

static inline CONST
float vec3_max_element(Vec3 a)
{
    return max_float(a.x,max_float(a.y,a.z));
}

static inline CONST
float vec3_min_element(Vec3 a)
{
    return min_float(a.x,min_float(a.y,a.z));
}

static inline CONST
Vec3 scale_vec3(Vec3 a, float f)
{
    return vec3(f*a.x,f*a.y,f*a.z);
}


static inline CONST
Vec3 invert_vec3(Vec3 a)
{
    return vec3(-a.x,-a.y,-a.z);
}

static inline CONST
Vec3 barycentric_comb_vec3(Vec3 a, Vec3 b, Vec3 c, float u, float v)
{
    Vec3 j = scale_vec3(a,1.f - u - v);
    Vec3 k = scale_vec3(b,u);
    Vec3 l = scale_vec3(c,v);
    return add_vec3(add_vec3(j,k),l);
}
