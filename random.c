#include "random.h"
#include "common.h"
#include <time.h>
void seed_rng(RNG *rng, uint64_t a, uint64_t b)
{
    pcg32_srandom_r(rng, a, b);
}
float random_sample(RNG *rng)
{
    uint32_t r;
    float val;
    do{
        r = pcg32_random_r(rng);
        val = (float)ldexp((double)r, -32);
    } while(val >= 1.f);
    return val;
}

Vec3 cosine_sample_hemisphere(float xi_1, float xi_2)
{
    float r = sqrtf(xi_1);
    float phi = (float)TWO_PI*xi_2;
    float a = r*cosf(phi);
    float b = r*sinf(phi);
    float c = sqrtf(1.f - xi_1); // == sqrtf(1.f - (a^2+b^2));
    Vec3 ret = vec3(a,b,c);
    ASSERT(fabsf(magnitude(ret) - 1.f) < EPSILON);
    return ret;
}
float cosine_sample_hemisphere_pdf(Vec3 v)
{
    return v.z*(float)INV_PI ;
}

Vec3 uniform_sample_hemisphere(float xi_1, float xi_2)
{
    float r = sqrtf(1.f - xi_1*xi_1);
    float phi = (float)TWO_PI*xi_2;
    return vec3(cosf(phi)*r,sinf(phi)*r,xi_1);
}

static uint64_t bad_seeds[] = {
        1453u, 1253525u,
        1550523u, 12u,
        389515u, 12814124u,
        124145u, 25646u,
        6092u, 95u,
        938403u, 9053253u,
        9804353u, 34958u,
        740325u, 8953753u,
        8793134u, 884u,
        29348u, 919u,
        1138u, 903123u,
        98231u, 7273u,
        10192u, 2323u,
        888483u, 3232811u,
        912321u, 23123u,
        833727u, 88u,
};

static unsigned num_bad_seeds = sizeof(bad_seeds)/sizeof(uint64_t)/2;
static unsigned used_bad_seeds = 0;

void init_rngs(RNG *rngs, unsigned num_rngs)
{
    uint64_t seeds[2];
    //TODO(Vidar): move this someplace else
#ifdef __linux__
    //TODO(Vidar): Nice seed on windows too.
    // Could probably use "CryptGenRandom"
    //TODO(Vidar): Move to random.c
    FILE *rand_f = fopen("/dev/urandom","r");
    if(rand_f){
        for(unsigned i=0;i<num_rngs;i++){
            fread(seeds,sizeof(seeds),1,rand_f);
            seed_rng(rngs+i,seeds[0],seeds[1]);
        }
        fclose(rand_f);
    } else {
        fprintf(stderr, "Error: could not open /dev/urandom, using worse"
                " seed for RNG\n");
        for(unsigned i=0;i<num_rngs;i++){
            uint64_t t = (uint64_t)time(NULL);
            seed_rng(rngs+i,t+bad_seeds[used_bad_seeds*2],t+bad_seeds[used_bad_seeds*2+1]);
            used_bad_seeds = (used_bad_seeds + 1 )%num_bad_seeds;
        }
    }
#endif
}
