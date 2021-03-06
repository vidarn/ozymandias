#pragma once
#include "common.h"
#include "exceptions.h"
#define PI      3.14159265358979323846
#define TWO_PI  6.28318530717958647692
#define HALF_PI 1.57079632679489661923
#define INV_PI  0.31830988618379067153
#define EPSILON 0.001

#define MIN_MAX(type) \
static inline \
type min_##type(type const x, type const y) \
{ return y < x ? y : x;} \
static inline \
type max_##type(type const x, type const y) \
{ return y > x ? y : x;}

MIN_MAX(u8)
MIN_MAX(u16)
MIN_MAX(u32)
MIN_MAX(u64)
MIN_MAX(s8)
MIN_MAX(s16)
MIN_MAX(s32)
MIN_MAX(s64)
MIN_MAX(float)
MIN_MAX(double)

#undef MIN_MAX

static inline CONST
float wrap_01(float const x)
{ return x - floorf(x); }

#define SWAP(a,b) do { __typeof__(a) _swap_tmp = a; a = b; b = _swap_tmp; } while (0)

