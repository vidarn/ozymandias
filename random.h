#pragma once
#include "vec3.h"
#include "libs/pcg/pcg_basic.h"

typedef pcg32_random_t RNG;
double random_sample(RNG *rng);
vec3 cosine_sample_hemisphere(float xi_1, float xi_2);
float cosine_sample_hemisphere_pdf(vec3 v);
vec3 uniform_sample_hemisphere(float xi_1, float xi_2);
void seed_rng(RNG *rng, uint64_t a, uint64_t b);
void init_rngs(RNG *rng, unsigned num); // Will seed from /dev/urandom or similar

