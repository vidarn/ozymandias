#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#define __USE_GNU
#include <fenv.h>
// Embree wrapper
#include "libs/embree/embree.h"

//OpenImageIO
//#include <OpenImageIO/imageio.h>
//#undef ASSERT // We have our own ASSERT macro...

#include "vec3.h"
#include "matrix.h"
#include "math_common.h"

#include "brdf.h"
#include "scene.h"

#include "scene.h"

#include "libs/thread/thread.h"
#include "common.h"
#include "random.h"

struct ShadeRec {
    vec3 p, n, col;
    float emit;
    float t;
    char hit;
};


// TODO(Vidar): Check page 63 in PBRT...
Matrix3 to_normal_matrix(vec3 n, vec3 tangent)
{
    vec3 binormal = normalize(cross(tangent, n));
    tangent = cross(n, binormal);
    Matrix3 ret = matrix3(tangent, binormal, n);
    return ret;
}

unsigned char to_pixel_value(float val)
{
    val = pow(val,0.454545f);
    unsigned char ret = (unsigned char)(255.f*(min(1.f,val)));
    return ret;
}

//TODO(Vidar) There's a terrible amount of parameters here...
vec3 light_sample(float a, float b, float c, vec3 p, float *inv_pdf,
        vec3 *color, Scene ozy_scene)
{
    int i = (int)(a*ozy_scene.num_light_tris);
    float sqrt_b = sqrtf(b);
    float u = 1.f - sqrt_b;
    float v = c*sqrt_b;
    int tri = ozy_scene.light_tris[i];
    int t1  = ozy_scene.tris[tri*3+0];
    int t2  = ozy_scene.tris[tri*3+1];
    int t3  = ozy_scene.tris[tri*3+2];
    vec3 v1 = ozy_scene.verts[t1];
    vec3 e1 = ozy_scene.verts[t2] - v1;
    vec3 e2 = ozy_scene.verts[t3] - v1;
    vec3 n1 = ozy_scene.normals[t1];
    vec3 n2 = ozy_scene.normals[t2];
    vec3 n3 = ozy_scene.normals[t3];
    vec3 light_n = (1.f - u - v)*n1 + u*n2 + v*n3;
    vec3 light_p = v1 + u*e1 + v*e2;
    vec3 dir = normalize(p - light_p);
    *inv_pdf  = ((float)ozy_scene.num_light_tris*2
            *magnitude(cross(e1,e2)))*(max(0.f,dot(light_n,dir)))
            /magnitude_sq(light_p - p);
    *color = ozy_scene.materials[ozy_scene.tri_material[tri]].emit;
    return light_p;
}

//TODO(Vidar) There's a terrible amount of parameters here...
vec3 direct_light(vec3 p, vec3 n, Matrix3 world2normal, vec3 exitant,
        EmbreeScene *scene, BRDF *brdf, Scene ozy_scene, RNG *rng)
{
    vec3 color={};
    if(ozy_scene.num_light_tris > 0){
        //TODO(Vidar) Struct for light sample?
        //TODO(Vidar) Do we need the light normal?
        float light_inv_pdf;
        vec3 light_color;

        vec3 light_p = light_sample(random_sample(rng), random_sample(rng),
                random_sample(rng), p, &light_inv_pdf, &light_color, ozy_scene);

        vec3 incident = light_p - p;
        float target_t = magnitude(incident);
        incident = normalize(incident);
        vec3 exitant_shade  = mul_matrix3(exitant, world2normal);
        vec3 incident_shade = mul_matrix3(incident, world2normal);
        vec3 attenuation = (vec3)(brdf->eval(exitant_shade,
                    incident_shade,brdf));
        Ray shadow_ray = {};
        embree_set_ray(&shadow_ray,p,incident,EPSILON,target_t-2.f*EPSILON);

        if(!embree_occluded(&shadow_ray,scene)){
            color = (max(0.f, dot(incident, n)))*light_color
                *attenuation*light_inv_pdf;
        }
    }
    return color;
}


    //TODO(Vidar): Read this from the command line instead...
#ifdef _WIN32
    const char *scene_filename = "c:/temp/scene.ozy";
    const char *png_filename  = "c:/temp/out.png";
    const char *out_filename  = "c:/temp/ozy_out";
#else
    const char *scene_filename = "/tmp/scene.ozy";
    const char *png_filename  = "/tmp/out_test.png";
    const char *out_filename  = "/tmp/ozy_out";
#endif

typedef struct 
{
    Scene ozy_scene;
    EmbreeScene *scene;
    int num_subsamples;
    RNG rng;
    int width;
    int height;
    float *pixels;
    int thread_id;
}RenderParams;

void render(RenderParams params)
{
    Scene        ozy_scene = params.ozy_scene;
    EmbreeScene *scene = params.scene;
    int          num_subsamples = params.num_subsamples;
    RNG          rng = params.rng;
    int          width = params.width;
    int          height = params.height;
    float       *pixels = params.pixels;

    //TODO(Vidar): Should be part of scene...
    const vec3 environment_color = (vec3){0.1f,0.1f,0.1f};

    /* -- Camera -- */
    float dx = (2.f*tanf(ozy_scene.camera.fov*.5f))/(float)width;
    float dy = (2.f*tanf(ozy_scene.camera.fov*.5f))/(float)height;

    /* -- Russian roulette --- */
    float termination_probability = 0.2f;
    float inv_termination_probability = 1.f/(1.f - termination_probability);

    float fy = -0.5f*dy*(float)height;
    for(int y=0;y<height;y++){
        float fx = -0.5f*dx*(float)width;
        for(int x=0;x<width;x++){
            for(int ss=0;ss<num_subsamples;ss++){
                float offset_x = dx*(-0.5f + random_sample(&rng));
                float offset_y = dy*(-0.5f + random_sample(&rng));
                vec3 offset = (vec3){fx+offset_x, -(fy+offset_y), -1.f};
                Ray ray = {};
                embree_set_ray(&ray,ozy_scene.camera.pos,normalize(
                        mul_matrix3(offset,ozy_scene.camera.transform)), 0.f,
                        FLT_MAX);
                vec3 radiance = (vec3)(0.f);
                int min_num_bounces = 3;
                char done = 0;
                vec3 path_attenuation = (vec3)(1.f);
                int bounce = 1;
                while(!done){
                    if(embree_intersect(&ray,scene)){
                        int id = ray.primID;
                        vec3 n = normalize((1.f - ray.u - ray.v)
                            * ozy_scene.normals[ozy_scene.tris[id*3+0]]
                            + ray.u * ozy_scene.normals[ozy_scene.tris[id*3+1]]
                            + ray.v * ozy_scene.normals[ozy_scene.tris[id*3+2]]);


                        vec3 v1 = ozy_scene.verts[ozy_scene.tris[id*3+0]];
                        vec3 v2 = ozy_scene.verts[ozy_scene.tris[id*3+1]];
                        vec3 v3 = ozy_scene.verts[ozy_scene.tris[id*3+2]];

                        // TODO(Vidar): Use uv coords instead if available...
                        vec3 tangent = normalize(v2 - v1);

                        Material material = ozy_scene.materials[
                            ozy_scene.tri_material[id]];
                        vec3 col = material.color;
                        vec3 p = ray.org + ray.dir*ray.tfar;

                        // world2normal transforms from world space
                        // to normal space
                        // That is, n * world2normal = (vec3){0,0,1}
                        // normal2world transforms from normal space to world
                        // space
                        // That is, (vec3){0,0,1} * world2normal = n
                        Matrix3 world2normal = to_normal_matrix(n, tangent);
                        Matrix3 normal2world = transpose(world2normal);
                        vec3 exitant = -1.f*(ray.dir);

                        // Length 0 light paths
                        if(bounce == 1){
                            radiance = radiance + material.emit;
                        }

                        path_attenuation = path_attenuation * col;
                        radiance = radiance + path_attenuation*
                            direct_light(p, n, world2normal, exitant,
                                scene,&material.brdf,ozy_scene,&rng);

                        vec3 exitant_shade = mul_matrix3(exitant, world2normal);
                        vec3 brdf_vec = material.brdf.sample(
                                random_sample(&rng), random_sample(&rng),
                                exitant_shade,&material.brdf);
                        //TODO(Vidar): Handle refraction...
                        if(brdf_vec.z > 0.f){
                            float brdf_pdf = material.brdf.sample_pdf(brdf_vec,
                                exitant_shade,&material.brdf);
                            embree_set_ray(&ray,p,
                                    mul_matrix3(brdf_vec, normal2world),
                                    EPSILON, FLT_MAX);
                            path_attenuation = path_attenuation
                                //TODO(Vidar): I think MAX evaluates the dot
                                // product twice...
                                *(1.f/brdf_pdf) * (max(0.f, dot(ray.dir, n)))
                                * material.brdf.eval(brdf_vec,exitant_shade,
                                       &material.brdf );
                        } else {
                            // Sample direction below the normal, sample wasted...
                            break;
                        }
                    } else {
                        radiance = radiance + path_attenuation
                            *environment_color;
                        break;
                    }
                    if(bounce > min_num_bounces){
                        float q = random_sample(&rng);
                        if(q < termination_probability){
                            break;
                        }
                        path_attenuation = inv_termination_probability
                            *path_attenuation;
                    }
                    bounce++;
                }
                pixels[(x + y*width)*3+0] += radiance.x;
                pixels[(x + y*width)*3+1] += radiance.y;
                pixels[(x + y*width)*3+2] += radiance.z;
            }
            fx += dx;
        }
        fy += dy;
    }
}

unsigned long thread_func(void *param)
{
    RenderParams *render_params = (RenderParams*)param;
    printf("Starting render thread %d!\n", render_params->thread_id);
    render(*render_params);
    return 0;
}

//TODO(Vidar): wrap OIIO
/*OIIO_NAMESPACE_USING

void write_pixel_buffer_to_file(const char *filename,int width, int height, 
        float *pixel_buffer, int num_subsamples)
{
    float inv_num_subsamples = 1.f/(float)num_subsamples;
    const int channels = 3;
    int pixel_buffer_size = width*height*channels;
    unsigned char *pixels = (unsigned char*)malloc(pixel_buffer_size);
    for(int j=0;j<pixel_buffer_size;j++){
        pixels[j] = to_pixel_value(pixel_buffer[j]*inv_num_subsamples);
        //pixels[j] = 255;
    }

    ImageOutput *out = ImageOutput::create (filename);
    if (out){
        ImageSpec spec (width, height, channels, TypeDesc::UINT8);
        out->open (filename, spec);
        out->write_image (TypeDesc::UINT8, pixels);
        out->close ();
        //ImageOutput::destroy (out);
    }
    free(pixels);
}*/

int main(int argc, char **argv)
{

    Scene ozy_scene = read_scene_file(scene_filename);
    //TODO(Vidar):read directly into RTCScene?
    EmbreeScene *scene = embree_init(ozy_scene);

    //NOTE(Vidar): Enable floating point exceptions
    ENABLE_FPE;
    //feenableexcept(FE_DIVBYZERO);

    /* -- Output image -- */
    const int width  = 512;
    const int height = 512;

    const int num_threads = 8;
    const int subsamples_per_thread = 3;
    int pixel_buffer_size = width*height*3;
    float *pixel_buffers
        = (float *)malloc(sizeof(float)*pixel_buffer_size*num_threads);
    memset(pixel_buffers,0,sizeof(float)*pixel_buffer_size*num_threads);

    ThreadHandle threads[num_threads];

    RenderParams render_params[num_threads] = {};

    uint64_t seeds[] = {
        1453u, 1253525u,
        1550523u, 12u,
        389515u, 12814124u,
        124145u, 25646u,
        6092u, 95u,
        938403u, 9053253u,
        9804353u, 34958u,
        740325u, 8953753u,
        934u, 884u,
    };
    ASSERT(num_threads <= sizeof(seeds)/2);

#ifdef __linux__
    //TODO(Vidar): Nice seed on windows too.
    // Could probably use "CryptGenRandom"
    //TODO(Vidar): Move to platform specific file
    FILE *rand_f = fopen("/dev/urandom","r");
    if(rand_f){
        fread(seeds,sizeof(seeds),1,rand_f);
        fclose(rand_f);
    } else {
        fprintf(stderr, "Error: could not open /dev/urandom, using worse"
                " seed for RNG\n");
    }
#endif
    for(int i=0;i<num_threads;i++){
        RNG rng = {};
        seed_rng(&rng,seeds[i*2],seeds[i*2 + 1]);

        render_params[i].ozy_scene = ozy_scene;
        render_params[i].scene = scene;
        render_params[i].num_subsamples = subsamples_per_thread;
        render_params[i].rng = rng;
        render_params[i].width = width;
        render_params[i].height = height;
        render_params[i].pixels = pixel_buffers + i*pixel_buffer_size;
        render_params[i].scene = scene;
        render_params[i].thread_id = i;
        threads[i] = start_thread(&thread_func,&(render_params[i]));
    }

    for(int i=0;i<num_threads;i++){
        wait_for_thread(threads[i]);
    }

    embree_close(scene);

    /*DEBUG(Vidar): check the output of each thread*/
    /*char buffer[512];
    for(int i=1;i<num_threads;i++){
        sprintf(buffer,"/tmp/thread_%d.png",i);
        write_pixel_buffer_to_file(buffer,width,height,
                pixel_buffers + i*pixel_buffer_size, subsamples_per_thread);
    }*/

    for(int i=1;i<num_threads;i++){
        for(int j=0;j<pixel_buffer_size;j++){
            pixel_buffers[j] += (pixel_buffers+i*pixel_buffer_size)[j];
        }
    }

    /*write_pixel_buffer_to_file(png_filename,width,height,pixel_buffers,
            subsamples_per_thread*num_threads);*/

    for(int j=0;j<pixel_buffer_size;j++){
        pixel_buffers[j] *= 1.f/(float)(subsamples_per_thread*num_threads);
    }
    FILE *f = fopen(out_filename,"wb");
    fwrite(pixel_buffers,sizeof(float),pixel_buffer_size,f);
    fclose(f);

    //show_image(width,height,pixels,argc,argv);

    printf("Done!\n");
}

