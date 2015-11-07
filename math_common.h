#pragma once
//#include <math.h>
//#include <stdlib.h>
#define PI      3.14159265358979323846
#define TWO_PI  6.28318530717958647692
#define HALF_PI 1.57079632679489661923
#define INV_PI  0.31830988618379067153
#define EPSILON 0.001

#define MIN_MAX(type) \
static inline \
type __attribute__((overloadable)) min(type const x, type const y) \
{ return y < x ? y : x;} \
static inline \
type __attribute__((overloadable)) max(type const x, type const y) \
{ return y > x ? y : x;}

MIN_MAX(int)
MIN_MAX(unsigned)
MIN_MAX(float)
MIN_MAX(double)

#undef MIN_MAX

#define ACTIVE_FPE FE_DIVBYZERO| FE_INVALID| FE_OVERFLOW //| FE_UNDERFLOW
#define ENABLE_FPE  feenableexcept (ACTIVE_FPE)
#define DISABLE_FPE fedisableexcept(ACTIVE_FPE)


