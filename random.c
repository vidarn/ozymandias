#include "random.h"
#include "common.h"
void seed_rng(RNG *rng, uint64_t a, uint64_t b)
{
    pcg32_srandom_r(rng, a, b);
}
double random_sample(RNG *rng)
{
    return ldexp((double)pcg32_random_r(rng), -32);
}

vec3 cosine_sample_hemisphere(float xi_1, float xi_2)
{
    float r = sqrtf(xi_1);
    float phi = (float)TWO_PI*xi_2;
    float a = r*cos(phi);
    float b = r*sin(phi);
    float c = sqrtf(1.f - xi_1); // == sqrtf(1.f - (a^2+b^2));
    vec3 ret = (vec3){a,b,c};
    ASSERT(fabsf(magnitude(ret) - 1.f) < EPSILON);
    return ret;
}
float cosine_sample_hemisphere_pdf(vec3 v)
{
    return (float)v.z*INV_PI ;
}

vec3 uniform_sample_hemisphere(float xi_1, float xi_2)
{
    float r = sqrtf(1.f - xi_1*xi_1);
    float phi = (float)TWO_PI*xi_2;
    return (vec3){cos(phi)*r,sin(phi)*r,xi_1};
}

