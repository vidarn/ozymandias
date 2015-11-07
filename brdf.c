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

float lambert_eval(vec3 omega_i, vec3 omega_o, BRDF *brdf)
{
    return (float)INV_PI;
}
vec3 lambert_sample(float xi_1, float xi_2, vec3 omega_o, BRDF *brdf)
{
    return cosine_sample_hemisphere(xi_1,xi_2);
}
float lambert_sample_pdf(vec3 omega_i, vec3 omega_o, BRDF *brdf)
{
    return cosine_sample_hemisphere_pdf(omega_i);
}

static inline float fresnel(float cos_theta, float n)
{
    float R0 = (1.f-n)/(1.f+n);
    R0 = R0*R0;
    return R0 + (1.f-R0)*pow(1.f-cos_theta,5.f);
}


float phong_eval(vec3 i, vec3 o, BRDF *brdf)
{
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    vec3 h = normalize(i+o);
    float h_dot_n = fabsf(h.z);
    float i_dot_n = fabsf(i.z);
    float o_dot_n = fabsf(o.z);
    float F = fresnel(i_dot_n,params->ior);
    float D = (params->shininess+2.f)/(float)TWO_PI
        *pow(h_dot_n,params->shininess);
    float a = 2.f*h_dot_n/fabsf(dot(h,o));
    float G = min(1.f,min(a*o_dot_n,a*i_dot_n));
    return D*G*F/(4*o_dot_n*i_dot_n);
}

vec3 phong_sample(float u, float v, vec3 omega_o, BRDF *brdf)
{
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    float cos_theta = powf(v,1.f/(params->shininess+1.f));
    //TODO(Vidar): use math.h function for this instead...
    float sin_theta = sqrtf(max(0.f,1.f - cos_theta*cos_theta));
    float phi   = (float)TWO_PI*u;
    vec3 h = (vec3){sin_theta*cos(phi), sin_theta*sin(phi), cos_theta};
    float dot_exitant_h = dot(omega_o,h);
    vec3 ret = -1.f*omega_o + 2.f*dot_exitant_h*h;
    return ret;
}

float phong_sample_pdf(vec3 omega_i, vec3 omega_o, BRDF *brdf)
{
    PhongParameters *params = (PhongParameters*)brdf->parameters;
    vec3 h = normalize(omega_i+omega_o);
    float cos_theta = fabsf(h.z);
    float pdf = ((params->shininess+1.f)*powf(cos_theta,params->shininess))
        /(4.f*(float)TWO_PI*dot(h,omega_o));
    return pdf;
}
