#include "brdf.h"
#include "math_common.h"
#include "common.h"
#include "random.h"
#include <stdlib.h>
BRDF get_lambert_brdf()
{
    BRDF ret   = {&lambert_eval, &lambert_sample, &lambert_sample_pdf, NULL,
        BRDF_TYPE_LAMBERT};
    return ret;
}

BRDF get_phong_brdf(float ior, float shininess)
{
    PhongParameters *params = (PhongParameters*)malloc(sizeof(PhongParameters));
    params->ior = ior;
    params->shininess = shininess;
    BRDF ret   = {&phong_eval, &phong_sample, &phong_sample_pdf, params,
        BRDF_TYPE_PHONG};
    return ret;
}

void free_brdf(BRDF brdf){
    switch(brdf.type){
        case BRDF_TYPE_PHONG:
            free((PhongParameters*)brdf.parameters);
            break;
        default:
            break;
    }
}

CONST float lambert_eval(UNUSED Vec3 omega_i, UNUSED Vec3 omega_o, UNUSED BRDF *brdf)
{
    return (float)INV_PI;
}
Vec3 lambert_sample(float xi_1, float xi_2, UNUSED Vec3 omega_o, UNUSED BRDF *brdf)
{
    return cosine_sample_hemisphere(xi_1,xi_2);
}
float lambert_sample_pdf(Vec3 omega_i, UNUSED Vec3 omega_o, UNUSED BRDF *brdf)
{
    return cosine_sample_hemisphere_pdf(omega_i);
}

static inline float fresnel(float cos_theta, float n)
{
    float R0 = (1.f-n)/(1.f+n);
    R0 = R0*R0;
    return R0 + (1.f-R0)*powf(1.f-cos_theta,5.f);
}


float phong_eval(Vec3 i, Vec3 o, BRDF *brdf)
{
    //TODO(Vidar): Check why we sometimes get o.z = 0...
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    Vec3 h = normalize(add_vec3(i,o));
    float h_dot_n = fabsf(h.z);
    float i_dot_n = fabsf(i.z);
    float o_dot_n = fabsf(o.z);
    float F = fresnel(i_dot_n,params->ior);
    float D = (params->shininess+2.f)/(float)TWO_PI
        *powf(h_dot_n,params->shininess);
    float a = 2.f*h_dot_n/fabsf(dot(h,o));
    float G = min_float(1.f,min_float(a*o_dot_n,a*i_dot_n));
    return D*G*F/(4*o_dot_n*i_dot_n);
}

Vec3 phong_sample(float u, float v, Vec3 omega_o, BRDF *brdf)
{
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    float cos_theta = powf(v,1.f/(params->shininess+1.f));
    //TODO(Vidar): use math.h function for this instead...
    float sin_theta = sqrtf(max_float(0.f,1.f - cos_theta*cos_theta));
    float phi   = (float)TWO_PI*u;
    Vec3 h = vec3(sin_theta*cosf(phi), sin_theta*sinf(phi), cos_theta);
    float dot_exitant_h = dot(omega_o,h);
    Vec3 tmp = scale_vec3(h,dot_exitant_h*2.f);
    Vec3 ret = add_vec3(invert_vec3(omega_o),tmp);
    return ret;
}

float phong_sample_pdf(Vec3 omega_i, Vec3 omega_o, BRDF *brdf)
{
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    Vec3 h = normalize(add_vec3(omega_i,omega_o));
    float cos_theta = fabsf(h.z);
    float pdf = ((params->shininess+1.f)*powf(cos_theta,params->shininess))
        /(4.f*(float)TWO_PI*dot(h,omega_o));
    return pdf;
}
