#include "path_tracer.h"
#include <stdlib.h>
#include <float.h>

// TODO(Vidar): This does not belong here
// TODO(Vidar): Check page 63 in PBRT...
static Matrix3 to_normal_matrix(Vec3 n, Vec3 tangent)
{
    Vec3 binormal = normalize(cross(tangent, n));
    tangent = cross(n, binormal);
    Matrix3 ret = matrix3(tangent, binormal, n);
    return ret;
}

//TODO(Vidar) There's a terrible amount of parameters here...
static Vec3 light_sample(float a, float b, float c, Vec3 p, float *inv_pdf,
        Vec3 *color, OzyScene scene)
{
    u32 i = (u32)(a*scene.light_tris.count);
    float sqrt_b = sqrtf(b);
    float u = 1.f - sqrt_b;
    float v = c*sqrt_b;
    u32 tri = scene.light_tris.data[i].tri;
    Object obj = scene.objects.data[scene.light_tris.data[i].obj];
    u32 t1  = obj.tris[tri*3+0];
    u32 t2  = obj.tris[tri*3+1];
    u32 t3  = obj.tris[tri*3+2];
    u32 nt1 = obj.tri_normals[tri*3+0];
    u32 nt2 = obj.tri_normals[tri*3+1];
    u32 nt3 = obj.tri_normals[tri*3+2];
    Vec3 v1 = obj.verts[t1];
    Vec3 e1 = sub_vec3(obj.verts[t2], v1);
    Vec3 e2 = sub_vec3(obj.verts[t3], v1);
    Vec3 n1 = obj.normals[nt1];
    Vec3 n2 = obj.normals[nt2];
    Vec3 n3 = obj.normals[nt3];
    Vec3 light_n = barycentric_comb_vec3(n1,n2,n3,u,v);
    Vec3 light_p;
    {
        Vec3 j = scale_vec3(e1,u);
        Vec3 k = scale_vec3(e2,v);
        light_p = add_vec3(add_vec3(j,k),v1);
    }
    Vec3 dir = normalize(sub_vec3(p,light_p));
    *inv_pdf  = ((float)scene.light_tris.count*2
            *magnitude(cross(e1,e2)))*(max_float(0.f,dot(light_n,dir)))
            /magnitude_sq(sub_vec3(light_p, p));
    *color = scene.materials.data[obj.tri_materials[tri]].emit;
    return light_p;
}

//TODO(Vidar) There's a terrible amount of parameters here...
static
Vec3 direct_light(Vec3 p, Vec3 n, Matrix3 world2normal, Vec3 exitant,
        EmbreeScene *embree_scene, BRDF *brdf, OzyScene scene,
        RNG *rng)
{
    Vec3 color={};
    if(scene.light_tris.count > 0){
        //TODO(Vidar) Struct for light sample?
        //TODO(Vidar) Do we need the light normal?
        float light_inv_pdf;
        Vec3 light_color;

        Vec3 light_p = light_sample((float)random_sample(rng),
                (float)random_sample(rng), (float)random_sample(rng),
                p, &light_inv_pdf, &light_color, scene);

        Vec3 incident = sub_vec3(light_p, p);
        float target_t = magnitude(incident);
        incident = normalize(incident);
        Vec3 exitant_shade  = mul_matrix3(world2normal, exitant);
        Vec3 incident_shade = mul_matrix3(world2normal, incident);
        float val = 0;
        if(incident_shade.z > EPSILON && exitant_shade.z > EPSILON){
            val = brdf->eval(exitant_shade, incident_shade,brdf);
        }
        Vec3 attenuation = vec3(val,val,val);
        Ray shadow_ray = {};
        embree_set_ray(&shadow_ray,p,incident,(float)EPSILON,
                target_t-2.f*(float)EPSILON);

        if(!embree_occluded(&shadow_ray,embree_scene)){
            color = scale_vec3(mul_vec3(light_color,attenuation),
                    max_float(0.f, dot(incident, n))*light_inv_pdf);
        }
    }
    return color;
}

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id)
{
    OzyScene scene              = params.scene;
    EmbreeScene *embree_scene   = params.embree_scene;
    u32          num_subsamples = params.num_subsamples;
    RNG          rng            = params.rng;

    //TODO(Vidar): Should be part of scene...
    const Vec3 environment_color = vec3(0.1f,0.1f,0.1f);

    Bucket bucket = bucket_grid.buckets[bucket_id];

    unsigned bucket_width  = bucket.max_x - bucket.min_x;

    /* -- Camera -- */
    float inv_max_width_height = 1.f/max_float((float)bucket_grid.width,
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
                //TODO(Vidar): Check these, do they really make sense?
                Vec4 cam_pos   = mul_matrix4(scene.camera.transform,
                        vec4(0.f,0.f,0.f,1.f));
                Vec4 offset    = mul_matrix4(scene.camera.transform,
                        vec4(fx+offset_x, -(fy+offset_y), 1.f, 0.f));
                Ray ray        = {};
                embree_set_ray(&ray,cam_pos,normalize(offset), 0.f,
                        FLT_MAX);
                Vec3 radiance         = vec3(0.f,0.f,0.f);
                u32 min_num_bounces   = 3;
                u8  done              = 0;
                Vec3 path_attenuation = vec3(1.f,1.f,1.f);
                u32 bounce            = 1;
                while(!done){
                    if(embree_intersect(&ray,embree_scene)){
                        u32 id = ray.primID;
                        Object obj = scene.objects.data[ray.geomID];
                        Vec3 n1 = obj.normals[obj.tri_normals[id*3+0]];
                        Vec3 n2 = obj.normals[obj.tri_normals[id*3+1]];
                        Vec3 n3 = obj.normals[obj.tri_normals[id*3+2]];
                        Vec3 n = barycentric_comb_vec3(n1,n2,n3,ray.u,ray.v);
                        Vec3 v1 = obj.verts[obj.tris[id*3+0]];
                        Vec3 v2 = obj.verts[obj.tris[id*3+1]];

                        // TODO(Vidar): Use uv coords instead if available...
                        Vec3 tangent = normalize(sub_vec3(v2, v1));

                        Material material = scene.materials.data[
                            obj.tri_materials[id]];
                        Vec3 col = material.color;
                        Vec3 p = add_vec3(ray.org,scale_vec3(ray.dir,ray.tfar));

                        // world2normal transforms from world space
                        // to normal space
                        // That is, n * world2normal = vec3(0,0,1)
                        // normal2world transforms from normal space to world
                        // space
                        // That is, vec3(0,0,1) * world2normal = n
                        Matrix3 world2normal = to_normal_matrix(n, tangent);
                        Matrix3 normal2world = transpose_matrix3(world2normal);
                        Vec3 exitant = invert_vec3(ray.dir);

                        // Length 0 light paths
                        if(bounce == 1){
                            radiance = add_vec3(radiance, material.emit);
                            // Store normal pass
                            // TODO(Vidar) use deep images for this?
                            // Makes no sense to antialias normals
                            ADD_PIXEL_SAMPLE_3(n,PASS_NORMAL);
                            ADD_PIXEL_SAMPLE_3(col,PASS_COLOR);
                            ADD_PIXEL_SAMPLE_1(ray.tfar,PASS_DEPTH);
                        }

                        path_attenuation = mul_vec3(path_attenuation, col);
                        radiance = add_vec3(radiance, mul_vec3(path_attenuation,
                            direct_light(p, n, world2normal, exitant,
                                embree_scene,&material.brdf,scene,&rng)));

                        Vec3 exitant_shade = mul_matrix3(world2normal, exitant);
                        Vec3 brdf_vec = material.brdf.sample(
                                (float)random_sample(&rng),
                                (float)random_sample(&rng),
                                exitant_shade,&material.brdf);
                        //TODO(Vidar): Handle refraction...
                        if(brdf_vec.z > 0.f){
                            float brdf_pdf = material.brdf.sample_pdf(brdf_vec,
                                exitant_shade,&material.brdf);
                            embree_set_ray(&ray,p,
                                    mul_matrix3(normal2world, brdf_vec),
                                    (float)EPSILON, FLT_MAX);
                            path_attenuation = scale_vec3(path_attenuation,
                                (1.f/brdf_pdf) * (max_float(0.f, dot(ray.dir, n)))
                                * material.brdf.eval(brdf_vec,exitant_shade,
                                       &material.brdf));
                        } else {
                            // Sample direction below the normal, sample wasted...
                            break;
                        }
                    } else {
                        radiance = add_vec3(radiance,mul_vec3(path_attenuation,
                            environment_color));
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
                        path_attenuation = scale_vec3(path_attenuation,
                                inv_termination_probability);
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

