#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <gsl/gsl_integration.h>
#include "cubature.h"
#include "math_common.h"
#include "matrix.h"
#include "vec3.h"
#include "brdf.h"
#include "common.h"
#include "random.h"
#include "libs/statistics/statistics.h"


static struct timeval tm1;

static inline void start_timer()
{
    gettimeofday(&tm1, NULL);
}

static inline unsigned long long stop_timer()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    return 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
}

enum
{
    TYPE_BOOL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
};

void print_table(void *cells, int xres, 
        int yres, const char *title, int digits, int type)   
{   
    printf(" %s:\n",title);   
    char buffer[digits+2];  
    memset(buffer,'-',digits+1); 
    buffer[digits+1] = 0; 
    for(int x=0;x<xres;x++){    
        printf("+%s",buffer);   
    }   
    printf("+\n");  
    for(int y=0;y<yres;y++){    
        for(int x=0;x<xres;x++){    
            switch(type){
                case TYPE_BOOL:
                    printf("|");  
                    for(int i=0;i<=digits;i++){
                        if(((int*)cells)[x + y*xres]){
                            printf("X");  
                        } else {
                            printf(" ");  
                        }
                    }
                    break;
                case TYPE_INT:
                    printf("| %*d", digits, ((int*)cells)[x + y*xres]);  
                    break;
                case TYPE_FLOAT:
                    printf("| %*.*f", digits, digits-2,
                            ((float*)cells)[x + y*xres]);  
                    break;
                case TYPE_DOUBLE:
                    printf("| %*.*f", digits, digits-2,
                            ((double*)cells)[x + y*xres]);  
                    break;
                default:
                    ASSERT(0);
            }
        }   
        printf("|\n");  
    }   
    for(int x=0;x<xres;x++){    
        printf("+%s",buffer);   
    }   
    printf("+\n");  
}   

typedef struct
{
    BRDF brdf;
    vec3 omega_o;
} FuncData;

int my_func_cub(unsigned ndim, const double *x, void *fdata, unsigned fdim,
        double *fval)
{
    FuncData *data = (FuncData*)fdata;
    double sin_theta_i = sin(x[0]);
    double cos_theta_i = cos(x[0]);
    vec3 omega_i = (vec3){cos(x[1])*sin_theta_i,
            sin(x[1])*sin_theta_i, cos_theta_i};
    *fval =  sin_theta_i*data->brdf.sample_pdf(omega_i,data->omega_o,
            &(data->brdf));
    return 0;
}


void chi_square_gof_test(BRDF brdf, int xres, int yres, vec3 omega_o, RNG *rng,
        int N, int verbose)
{
    //TODO(Vidar): clean up, optimize (sse?)
    int n = xres * yres;
    int bins[n];
    double p[n];
    memset(bins,0,sizeof(int)*n);
    memset(p,0,sizeof(double)*n);

    float theta_max = HALF_PI;
    float phi_max   = TWO_PI;
    double phi_step   = phi_max/(double)xres;
    double theta_step = theta_max/(double)yres;

    for(int i =0;i < N;i++){
        vec3 omega_i = brdf.sample(random_sample(rng),random_sample(rng),
                omega_o,&brdf);
        if(omega_i.z > 0.f){
            float theta_i = theta_max - asinf(omega_i.z);
            float phi_i   = atan2f(omega_i.y,omega_i.x);
            phi_i = (phi_i < 0.f ? phi_i+TWO_PI : phi_i);
            float x = phi_i/phi_max;
            float y = theta_i/theta_max;
            int t = min((int)(x*(double)xres), xres-1);
            int p = min((int)(y*(double)yres), yres-1);
            bins[t + p*xres]++;
        }
    }

    FuncData func_data = {brdf,omega_o};
    double theta = 0.f;
    for(int y = 0; y < yres; y++){
        double phi = 0.f;
        for(int x = 0; x < xres; x++){
            double result, abserr;
            double min_limit[] = {theta, phi};
            double max_limit[] = {theta+theta_step, phi+phi_step};
            hcubature(1, &my_func_cub, &func_data, 2, min_limit, max_limit, 0, 0., 1e-3,
                          ERROR_L1, p + x + y*xres, &abserr);
            phi += phi_step;
        }
        theta += theta_step;
    }

    double inv_num_bins = 1.0/(double)N;
    double d_bins[n];
    int sum = 0;
    double p_sum = 0.f;
    for(int i=0;i<n;i++){
        d_bins[i] = (double)bins[i]*inv_num_bins;
        sum += bins[i];
        p_sum += p[i];
    }

    int valid_bins[n];
    for(int i =0;i<n;i++){
        double expected = (double)N*p[i];
        valid_bins[i] = expected > 5.0;
    }

    int dof = -1; // The degrees of freedom is equal to the number of used bins -1

    // \chi^2 = N \sum_{i=1}^n p_i \frac{(O_i/N - p_i)^2}{p_i}
    // where N is the number of observations
    //       O_i is the number of observations in bin i
    //       p_i is the theoretical frequency of bin i
    //       n   is the number of bins
    double chi_2 = 0.;
    for(int i=0;i<n;i++){
        double expected = (double)N*p[i];
        if(expected > 5.0){
            double tmp = (bins[i] - expected);
            chi_2 += tmp*tmp/expected;
            dof++;
        }
    }

    double p_value = 1.0 - chi2_cdf(chi_2,dof);

    if(verbose){
        print_table(d_bins,xres,yres,"d_bins",8,TYPE_DOUBLE);
        print_table(p,xres,yres,"p",8,TYPE_DOUBLE);
        printf("p sum:\t%15.13f\n\n",p_sum);
        print_table(bins,xres,yres,"bins",5,TYPE_INT);
        printf("sum:\t%d\n\n",sum);
        print_table(valid_bins,xres,yres,"valid bins",5,TYPE_BOOL);
    }

    printf("p-value = %f\n",p_value);
}

void test_brdf_sample_uniform_length()
{
    RNG rng;
    seed_rng(&rng,time(NULL),time(NULL));

    for(;;){
        BRDF brdf = get_phong_brdf(15.f*random_sample(&rng)
                ,500.f*random_sample(&rng));
        float theta = HALF_PI*random_sample(&rng);
        float phi   = TWO_PI *random_sample(&rng);
        float cos_theta = cos(theta);
        float sin_theta = sin(theta);
        vec3 omega_o = (vec3){sin_theta*cos(phi),sin_theta*sin(phi),
                    cos_theta};
        ASSERT(fabsf(magnitude(omega_o) - 1.f) < EPSILON);
        float u = random_sample(&rng);
        float v = random_sample(&rng);
        vec3 brdf_vec = brdf.sample(u,v,omega_o,&brdf);
        ASSERT(fabsf(magnitude(brdf_vec) - 1.f) < EPSILON);
    }
}

void test_brdf_sample_pdf()
{
    BRDF brdf = get_phong_brdf(1.5f,50.f);
    //BRDF brdf = get_lambert_brdf();
    RNG rng;
    seed_rng(&rng,time(NULL),time(NULL));
    int theta_steps = 10;
    int phi_steps = 1;
    for(int t = 0; t < theta_steps; t++){
        float theta = HALF_PI*(float)t/(float)theta_steps;
        float cos_theta = cos(theta);
        float sin_theta = sin(theta);
        for(int p = 0; p < phi_steps; p++){
            float phi   = TWO_PI*(float)p/(float)phi_steps;
            vec3 omega_o = (vec3){sin_theta*cos(phi),sin_theta*sin(phi),cos_theta};
            chi_square_gof_test(brdf,11,20,omega_o,&rng,100000,0);
        }
    }
}

Matrix3 to_normal_matrix(vec3 n, vec3 tangent)
{
    vec3 binormal = normalize(cross(n, tangent));
    tangent = cross(n, binormal);
    Matrix3 ret = matrix3(invert(tangent), binormal, n);
    return ret;
}

void test_normal_matrix()
{
    vec3 n = (vec3){0.f, 0.f, 1.f};
    vec3 t = normalize((vec3){1.f, 2.f, 0.f});
    Matrix3 inv_shade_matrix = to_normal_matrix(n,t);
    Matrix3 shade_matrix = transpose(inv_shade_matrix);
    printf("invserse shade matrix:\n");
    print(inv_shade_matrix);
    printf("shade matrix:\n");
    print(shade_matrix);
    vec3 a = mul_matrix3((vec3){1.f, 0.f, 0.f}, shade_matrix);
    vec3 b = mul_matrix3((vec3){0.f, 1.f, 0.f}, shade_matrix);
    vec3 c = mul_matrix3((vec3){0.f, 0.f, 1.f}, shade_matrix);
    printf("\n");
    print(a);
    print(t);
    printf("\n");
    print(b);
    printf("\n");
    print(c);
    print(n);
    printf("\n");
}

int main(int argc, char **argv){
    test_brdf_sample_pdf();
    //test_brdf_sample_uniform_length();
    test_normal_matrix();
    return 0;
}
