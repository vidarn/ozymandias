#include "path_tracer.h"
#include <stdlib.h>
#include <float.h>

// TODO(Vidar): This does not belong here
// TODO(Vidar): Check page 63 in PBRT...
static Matrix3 to_normal_matrix(vec3 n, vec3 tangent)
{
    vec3 binormal = normalize(cross(tangent, n));
    tangent = cross(n, binormal);
    Matrix3 ret = matrix3(tangent, binormal, n);
    return ret;
}

//TODO(Vidar) There's a terrible amount of parameters here...
static vec3 light_sample(float a, float b, float c, vec3 p, float *inv_pdf,
        vec3 *color, OzyScene scene)
{
    u32 i = (u32)(a*scene.num_light_tris);
    float sqrt_b = sqrtf(b);
    float u = 1.f - sqrt_b;
    float v = c*sqrt_b;
    u32 tri = scene.light_tris[i];
    u32 t1  = scene.tris[tri*3+0];
    u32 t2  = scene.tris[tri*3+1];
    u32 t3  = scene.tris[tri*3+2];
    vec3 v1 = scene.verts[t1];
    vec3 e1 = scene.verts[t2] - v1;
    vec3 e2 = scene.verts[t3] - v1;
    vec3 n1 = scene.normals[t1];
    vec3 n2 = scene.normals[t2];
    vec3 n3 = scene.normals[t3];
    vec3 light_n = (1.f - u - v)*n1 + u*n2 + v*n3;
    vec3 light_p = v1 + u*e1 + v*e2;
    vec3 dir = normalize(p - light_p);
    *inv_pdf  = ((float)scene.num_light_tris*2
            *magnitude(cross(e1,e2)))*(max(0.f,dot(light_n,dir)))
            /magnitude_sq(light_p - p);
    *color = scene.materials[scene.tri_material[tri]].emit;
    return light_p;
}

//TODO(Vidar) There's a terrible amount of parameters here...
static
vec3 direct_light(vec3 p, vec3 n, Matrix3 world2normal, vec3 exitant,
        EmbreeScene *embree_scene, BRDF *brdf, OzyScene scene,
        RNG *rng)
{
    vec3 color={};
    if(scene.num_light_tris > 0){
        //TODO(Vidar) Struct for light sample?
        //TODO(Vidar) Do we need the light normal?
        float light_inv_pdf;
        vec3 light_color;

        vec3 light_p = light_sample((float)random_sample(rng),
                (float)random_sample(rng), (float)random_sample(rng),
                p, &light_inv_pdf, &light_color, scene);

        vec3 incident = light_p - p;
        float target_t = magnitude(incident);
        incident = normalize(incident);
        vec3 exitant_shade  = mul_matrix3(exitant, world2normal);
        vec3 incident_shade = mul_matrix3(incident, world2normal);
        vec3 attenuation = (vec3)(brdf->eval(exitant_shade,
                    incident_shade,brdf));
        Ray shadow_ray = {};
        embree_set_ray(&shadow_ray,p,incident,(float)EPSILON,
                target_t-2.f*(float)EPSILON);

        if(!embree_occluded(&shadow_ray,embree_scene)){
            color = (max(0.f, dot(incident, n)))*light_color
                *attenuation*light_inv_pdf;
        }
    }
    return color;
}

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id)
{
    OzyScene scene      = params.scene;
    EmbreeScene *embree_scene   = params.embree_scene;
    u32          num_subsamples = params.num_subsamples;
    RNG          rng            = params.rng;

    //TODO(Vidar): Should be part of scene...
    const vec3 environment_color = (vec3){0.1f,0.1f,0.1f};

    Bucket bucket = bucket_grid.buckets[bucket_id];

    unsigned bucket_width  = bucket.max_x - bucket.min_x;

    /* -- Camera -- */
    float inv_max_width_height = 1.f/max((float)bucket_grid.width,
            (float)bucket_grid.height);
    float dx = (2.f*tanf(scene.camera.fov*.5f))*inv_max_width_height;
    float dy = (2.f*tanf(scene.camera.fov*.5f))*inv_max_width_height;

    /* -- Russian roulette --- */
    float termination_probability = 0.2f;
    float inv_termination_probability = 1.f/(1.f - termination_probability);

    float fy = dy*((-0.5f)*(float)bucket_grid.height+(float)bucket.min_y);
    for(u32 y = bucket.min_y;y<bucket.max_y;y++){
        float fx = dx*((-0.5f)*(float)bucket_grid.width+(float)bucket.min_x);
        for(u32 x= bucket.min_x;x<bucket.max_x;x++){
            for(u32 ss=0;ss<num_subsamples;ss++){
                float offset_x = dx*(-0.5f + (float)random_sample(&rng));
                float offset_y = dy*(-0.5f + (float)random_sample(&rng));
                vec3 offset    = (vec3){fx+offset_x, -(fy+offset_y), -1.f};
                Ray ray        = {};
                embree_set_ray(&ray,scene.camera.pos,normalize(
                        mul_matrix3(offset,scene.camera.transform)), 0.f,
                        FLT_MAX);
                vec3 radiance         = (vec3)(0.f);
                u32 min_num_bounces   = 3;
                u8  done              = 0;
                vec3 path_attenuation = (vec3)(1.f);
                u32 bounce            = 1;
                while(!done){
                    if(embree_intersect(&ray,embree_scene)){
                        u32 id = ray.primID;
                        vec3 n = normalize((1.f - ray.u - ray.v)
                            * scene.normals[scene.tris[id*3+0]]
                            + ray.u * scene.normals[scene.tris[id*3+1]]
                            + ray.v * scene.normals[scene.tris[id*3+2]]);


                        vec3 v1 = scene.verts[scene.tris[id*3+0]];
                        vec3 v2 = scene.verts[scene.tris[id*3+1]];

                        // TODO(Vidar): Use uv coords instead if available...
                        vec3 tangent = normalize(v2 - v1);

                        Material material = scene.materials[
                            scene.tri_material[id]];
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
                            // Store normal pass
                            // TODO(Vidar) use deep images for this?
                            // Makes no sense to antialias normals
                            ADD_PIXEL_SAMPLE_3(n,PASS_NORMAL);
                            ADD_PIXEL_SAMPLE_3(col,PASS_COLOR);
                            ADD_PIXEL_SAMPLE_1(ray.tfar,PASS_DEPTH);
                        }

                        path_attenuation = path_attenuation * col;
                        radiance = radiance + path_attenuation*
                            direct_light(p, n, world2normal, exitant,
                                embree_scene,&material.brdf,scene,&rng);

                        vec3 exitant_shade = mul_matrix3(exitant, world2normal);
                        vec3 brdf_vec = material.brdf.sample(
                                (float)random_sample(&rng),
                                (float)random_sample(&rng),
                                exitant_shade,&material.brdf);
                        //TODO(Vidar): Handle refraction...
                        if(brdf_vec.z > 0.f){
                            float brdf_pdf = material.brdf.sample_pdf(brdf_vec,
                                exitant_shade,&material.brdf);
                            embree_set_ray(&ray,p,
                                    mul_matrix3(brdf_vec, normal2world),
                                    (float)EPSILON, FLT_MAX);
                            path_attenuation = path_attenuation
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
                        if(bounce == 1){
                            ADD_PIXEL_SAMPLE_3(environment_color,PASS_COLOR);
                            ADD_PIXEL_SAMPLE_1(INFINITY,PASS_DEPTH);
                        }
                        break;
                    }
                    if(bounce > min_num_bounces){
                        float q = (float)random_sample(&rng);
                        if(q < termination_probability){
                            break;
                        }
                        path_attenuation = inv_termination_probability
                            *path_attenuation;
                    }
                    bounce++;
                }
                //TODO(Vidar): Handle alpha
                ADD_PIXEL_SAMPLE_4(radiance,1.f,PASS_FINAL);
            }
            fx += dx;
        }
        fy += dy;
    }
}

