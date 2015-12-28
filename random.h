#pragma once
#include "vec3.h"
#include "libs/pcg/pcg_basic.h"

typedef pcg32_random_t RNG;
float random_sample(RNG *rng);
Vec3 cosine_sample_hemisphere(float xi_1, float xi_2);
float cosine_sample_hemisphere_pdf(Vec3 v);
Vec3 uniform_sample_hemisphere(float xi_1, float xi_2);
void seed_rng(RNG *rng, uint64_t a, uint64_t b);
void init_rngs(RNG *rng, unsigned num); // Will seed from /dev/urandom or similar


