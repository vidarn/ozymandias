#pragma once
#include "vec3.h"
#include "common.h"
enum BRDF_TYPE
{
    BRDF_TYPE_PHONG,
    BRDF_TYPE_LAMBERT,
    BRDF_TYPE_EMIT, // TODO(Vidar): start using this... ;)
    BRDF_TYPE_COUNT
};

struct BRDF
{
    float (*eval)(Vec3 omega_i, Vec3 omega_o, struct BRDF *brdf);
    Vec3  (*sample)(float xi_1, float xi_2, Vec3 omega_o,
            struct BRDF *brdf);
    float (*sample_pdf)(Vec3 omega_i, Vec3 omega_o, struct BRDF *brdf);
    void *parameters;
    enum BRDF_TYPE type;
};
typedef struct BRDF BRDF;

void free_brdf(BRDF brdf);

BRDF get_lambert_brdf(void);
CONST float lambert_eval(Vec3 omega_i, Vec3 omega_o, BRDF *brdf);
Vec3 lambert_sample(float xi_1, float xi_2,
        Vec3 omega_o, BRDF *brdf);
float lambert_sample_pdf(Vec3 omega_i, Vec3 omega_o, BRDF *brdf);

BRDF get_phong_brdf(float ior, float shininess);
typedef struct
{
    float ior, shininess;
} PhongParameters;

float phong_eval(Vec3 omega_i, Vec3 omega_o, BRDF *brdf);
Vec3 phong_sample(float xi_1, float xi_2,
        Vec3 omega_o, BRDF *brdf);
float phong_sample_pdf(Vec3 omega_i, Vec3 omega_o, BRDF *brdf);

