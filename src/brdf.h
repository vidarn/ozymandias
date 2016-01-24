#pragma once
#include "vec3.h"
#include "common.h"
#include "brdf_types.h"

float brdf_eval(Vec3 omega_i, Vec3 omega_o, enum BRDF_TYPE type, void *param);
Vec3  brdf_sample(float xi_1, float xi_2, Vec3 omega_o,
        enum BRDF_TYPE type, void *param);
float brdf_sample_pdf(Vec3 omega_i, Vec3 omega_o, enum BRDF_TYPE type, void *param);

