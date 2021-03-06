#include "path_tracer.h"
#include <stdlib.h>
#include <float.h>
#include "dynamic_array.h"
#include "brdf.h"

DYNAMIC_ARRAY_IMP(OSL_Closure)

//TODO(Vidar): Should be part of scene...
// Use shader for environment
//static const Vec3 environment_color = {{0.2f,0.2f,0.2f}};
static const Vec3 environment_color = {{0.f,0.f,0.f}};

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
// Make a "context" struct or similar...
static Vec3 light_sample(float a, float b, float c, Vec3 p, float *inv_pdf,
        Vec3 *color, OzyScene scene, OSL_ShadingSystem *shading_system,
        OSL_ThreadContext *osl_thread_context)
{
    *color = vec3(0.f,0.f,0.f);
    if(scene.light_tris.count > 1){
        u32 start = 0;
        u32 end = scene.light_tris.count;
        u32 i = (end+start)/2;
        while(scene.light_tris.data[i-1].cdf > a
                    || scene.light_tris.data[i].cdf < a){
            if(scene.light_tris.data[i-1].cdf > a){
                end = i;
            }else{
                start = i;
            }
            i = (end+start)/2;
        }
        LightTri light_tri = scene.light_tris.data[i];
        float sqrt_b = sqrtf(b);
        float u = 1.f - sqrt_b;
        float v = c*sqrt_b;
        u32 tri = light_tri.tri;
        float area = light_tri.area;
        float pmf  = light_tri.pmf;
        Object obj = scene.objects.data[light_tri.obj];
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
        u32 material_id = obj.tri_materials[tri];
        Vec3 light_n = barycentric_comb_vec3(n1,n2,n3,u,v);
        Vec3 light_p;
        {
            Vec3 j = scale_vec3(e1,u);
            Vec3 k = scale_vec3(e2,v);
            light_p = add_vec3(add_vec3(j,k),v1);
        }
        Vec3 dir = normalize(sub_vec3(p,light_p));
        *inv_pdf  = area/pmf*
            fabsf(dot(light_n,dir))/magnitude_sq(sub_vec3(light_p, p));

        //TODO(Vidar): Unify...
        OSL_ShadeRec shade_rec = {};

        //TODO(Vidar): UV coordinates on the light...
        shade_rec.u = 0.5f;
        shade_rec.v = 0.5f;

        shade_rec.P[0] = light_p.x;
        shade_rec.P[1] = light_p.y;
        shade_rec.P[2] = light_p.z;

        shade_rec.N[0] = light_n.x;
        shade_rec.N[1] = light_n.y;
        shade_rec.N[2] = light_n.z;

        shade_rec.surfacearea = area;

        DynArr_OSL_Closure closures = osl_shade(shading_system,
                osl_thread_context,&shade_rec,material_id);
        for(u32 ii = 0;ii<closures.count;ii++){
            const OSL_Closure closure = closures.data[ii];
            if(closure.type == BRDF_TYPE_EMIT){
                *color = add_vec3(*color,vec3(closure.color[0],closure.color[1],
                    closure.color[2]));
            }
        }
        osl_free_closures(closures);
        return light_p;
    }else{
        return vec3(1.f,0.f,0.f);
    }
}

//TODO(Vidar) There's a terrible amount of parameters here...
static inline
Vec3 direct_light(Vec3 p, Vec3 n, Matrix3 world2normal, Matrix3 normal2world,
        Vec3 exitant, EmbreeScene *embree_scene, DynArr_OSL_Closure closures,
        OzyScene scene, RNG *rng, enum OzyDirectLightSampling sampling,
        OSL_ShadingSystem *shading_system, OSL_ThreadContext *osl_thread_context
        )
{
    Vec3 color={};
    if(scene.light_tris.count > 0){
        //TODO(Vidar) Struct for light sample?
        //TODO(Vidar) Do we need the light normal?
        Vec3 light_sample_color={};
        Vec3 brdf_sample_color={};

        float light_inv_pdf;
        float brdf_pdf;

        OSL_Closure sample_closure;
        float total_weight = 0.f;
        for(u32 i = 0;i<closures.count;i++){
            total_weight += closures.data[i].weight;
        }
        float r = random_sample(rng)*total_weight;
        for(u32 i = 0;i<closures.count;i++){
            r -= closures.data[i].weight;
            if(r <= 0.f){
                sample_closure = closures.data[i];
                break;
            }
        }
        float closure_inv_prob = total_weight/sample_closure.weight;
        //NOTE(Vidar): Sample the light source
        {
            Vec3 light_color;

            Vec3 light_p = light_sample((float)random_sample(rng),
                    (float)random_sample(rng), (float)random_sample(rng),
                    p, &light_inv_pdf, &light_color, scene, shading_system,
                    osl_thread_context);

            Vec3 incident = sub_vec3(light_p, p);
            float target_t = magnitude(incident);
            incident = normalize(incident);
            Vec3 exitant_shade  = mul_matrix3(world2normal, exitant);
            Vec3 incident_shade = mul_matrix3(world2normal, incident);
            Vec3 attenuation = vec3(0.f,0.f,0.f);
            if(incident_shade.z > EPSILON && exitant_shade.z > EPSILON){
                float val = brdf_eval(exitant_shade, incident_shade,
                    sample_closure.type, sample_closure.param);
                Vec3 col = vec3(val*sample_closure.color[0],
                    val*sample_closure.color[1], val*sample_closure.color[2]);
                attenuation = add_vec3(attenuation,col);
            }
            Ray shadow_ray = {};
            embree_set_ray(&shadow_ray,p,incident,(float)EPSILON,
                    target_t-2.f*(float)EPSILON);

            if(!embree_occluded(&shadow_ray,embree_scene)){
                light_sample_color = scale_vec3(mul_vec3(light_color,attenuation),
                        max_float(0.f, dot(incident, n))*light_inv_pdf*
                    closure_inv_prob);
            }
        }
        //NOTE(Vidar): Sample the brdf
        {
            Vec3 exitant_shade  = mul_matrix3(world2normal, exitant);
            Vec3 incident_shade = normalize(brdf_sample(
                    (float)random_sample(rng),
                    (float)random_sample(rng),
                    exitant_shade,sample_closure.type,sample_closure.param));
            brdf_pdf = brdf_sample_pdf(incident_shade,
                exitant_shade,sample_closure.type,sample_closure.param);
            if(incident_shade.z > EPSILON && exitant_shade.z > EPSILON &&
                    brdf_pdf > EPSILON){
                Ray ray = {};
                Vec3 incident = mul_matrix3(normal2world, incident_shade);
                embree_set_ray(&ray,p,incident
                        ,(float)EPSILON, FLT_MAX);
                if(embree_intersect(&ray,embree_scene)){
                    u32 id = ray.primID;
                    Object obj = scene.objects.data[ray.geomID];
                    u32 material_id = obj.tri_materials[id];

                    //TODO(Vidar): Fill the ShadeRec
                    OSL_ShadeRec shade_rec = {};

                    Vec3 light_color = vec3(0.f,0.f,0.f);

                    DynArr_OSL_Closure light_closures = osl_shade(shading_system,
                            osl_thread_context,&shade_rec,material_id);
                    for(u32 ii = 0;ii<light_closures.count;ii++){
                        const OSL_Closure light_closure =
                            light_closures.data[ii];
                        if(light_closure.type == BRDF_TYPE_EMIT){
                            light_color = add_vec3(light_color,
                                vec3(light_closure.color[0],
                                     light_closure.color[1],
                                     light_closure.color[2]));
                        }
                    }
                    osl_free_closures(light_closures);

                    float val = brdf_eval(exitant_shade, incident_shade,
                        sample_closure.type, sample_closure.param);
                    Vec3 attenuation = vec3(val*sample_closure.color[0],
                        val*sample_closure.color[1],
                        val*sample_closure.color[2]);
                    brdf_sample_color = scale_vec3(
                        mul_vec3(light_color,attenuation),
                        max_float(0.f, dot(incident, n))
                        /brdf_pdf*closure_inv_prob);
                }
            }
        }
        float val_light = light_inv_pdf > EPSILON ?
            1.f/(light_inv_pdf) : 0.f;
        float val_brdf = brdf_pdf;
        if(val_light > EPSILON  || val_brdf > EPSILON){
            //NOTE(Vidar):Power heuristic
            float mul = 1.f/(val_light*val_light + val_brdf*val_brdf);
            float weight_light = val_light*val_light * mul;
            float weight_brdf  = val_brdf*val_brdf   * mul;
            switch(sampling){
                case OZY_DIRECT_LIGHT_SAMPLING_BOTH:
                    color = add_vec3(scale_vec3(light_sample_color,weight_light),
                                scale_vec3(brdf_sample_color,weight_brdf));
                    break;
                case OZY_DIRECT_LIGHT_SAMPLING_LIGHT:
                    color = light_sample_color;
                    break;
                case OZY_DIRECT_LIGHT_SAMPLING_BRDF:
                    color = brdf_sample_color;
                    break;
            }
        }
    }
    return color;
}

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id)
{
    OzyScene scene                    = params.scene;
    EmbreeScene *embree_scene         = params.embree_scene;
    OSL_ShadingSystem *shading_system = params.shading_system;
    u32          num_subsamples       = params.num_subsamples;
    RNG          rng                  = params.rng;
    enum OzyDirectLightSampling direct_light_sampling
        = params.direct_light_sampling;

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

    /* -- OSL -- */
    OSL_ThreadContext *osl_thread_context =
        osl_get_thread_context(shading_system);

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
                        float uv1_s = obj.uvs[obj.tri_uvs[id*3+0]*2+0];
                        float uv1_t = obj.uvs[obj.tri_uvs[id*3+0]*2+1];
                        float uv2_s = obj.uvs[obj.tri_uvs[id*3+1]*2+0];
                        float uv2_t = obj.uvs[obj.tri_uvs[id*3+1]*2+1];
                        float uv3_s = obj.uvs[obj.tri_uvs[id*3+2]*2+0];
                        float uv3_t = obj.uvs[obj.tri_uvs[id*3+2]*2+1];
                        Vec3 n = barycentric_comb_vec3(n1,n2,n3,ray.u,ray.v);
                        Vec3 v1 = obj.verts[obj.tris[id*3+0]];
                        Vec3 v2 = obj.verts[obj.tris[id*3+1]];
                        Vec3 v3 = obj.verts[obj.tris[id*3+2]];

                        // TODO(Vidar): Use uv coords instead...
                        Vec3 tangent = normalize(sub_vec3(v2, v1));

                        u32 material_id = obj.tri_materials[id];
                        Vec3 p = add_vec3(ray.org,scale_vec3(ray.dir,ray.tfar));

                        OSL_ShadeRec shade_rec = {};
                        shade_rec.u = 0.5f;
                        shade_rec.v = 0.5f;
                        if(obj.num_uvs > 0){
                            float u = ray.u*uv2_s+ray.v*uv3_s
                                    +(1.f-ray.u-ray.v)*uv1_s;
                            float v = ray.u*uv2_t+ray.v*uv3_t
                                    +(1.f-ray.u-ray.v)*uv1_t;
                            shade_rec.u = wrap_01(u);
                            shade_rec.v = wrap_01(v);
                        }
                        shade_rec.P[0] = p.x;
                        shade_rec.P[1] = p.y;
                        shade_rec.P[2] = p.z;

                        shade_rec.N[0] = n.x;
                        shade_rec.N[1] = n.y;
                        shade_rec.N[2] = n.z;

                        shade_rec.I[0] = ray.dir.x;
                        shade_rec.I[1] = ray.dir.y;
                        shade_rec.I[2] = ray.dir.z;

                        //TODO(Vidar): According to the OSL spec. we actually
                        // only have to do this for emissive tris...
                        shade_rec.surfacearea = magnitude(cross(sub_vec3(v2,v1),
                                    sub_vec3(v3,v1)));

                        DynArr_OSL_Closure closures = osl_shade(shading_system,
                                osl_thread_context,&shade_rec,material_id);

                        if(closures.count == 0){
                            // Invalid closure, abort
                            break;
                        }
                        OSL_Closure closure = closures.data[0];
                        float total_weight = 0.f;
                        for(u32 i = 0;i<closures.count;i++){
                            total_weight += closures.data[i].weight;
                        }
                        float r = random_sample(&rng)*total_weight;
                        for(u32 i = 0;i<closures.count;i++){
                            r -= closures.data[i].weight;
                            if(r <= 0.f){
                                closure = closures.data[i];
                                break;
                            }
                        }
                        float closure_inv_prob = total_weight/closure.weight;
                        Vec3 col = vec3(closure.color[0],closure.color[1],
                            closure.color[2]);

                        //TODO(Vidar):Use the normal from OSL instead!

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
                            if(closure.type == BRDF_TYPE_EMIT){
                                radiance = add_vec3(radiance,
                                    vec3(closure.color[0], closure.color[1],
                                    closure.color[2]));
                            }
                            // Store normal pass
                            // TODO(Vidar) use deep images for this?
                            // Makes no sense to antialias normals
                            ADD_PIXEL_SAMPLE_3(n,PASS_NORMAL);
                            ADD_PIXEL_SAMPLE_3(col,PASS_COLOR);
                            ADD_PIXEL_SAMPLE_1(ray.tfar,PASS_DEPTH);
                        }

                        radiance = add_vec3(radiance, mul_vec3(path_attenuation,
                            direct_light(p, n, world2normal, normal2world,
                                exitant, embree_scene,closures,scene,&rng,
                                direct_light_sampling,shading_system,
                                osl_thread_context)));
                        path_attenuation = mul_vec3(path_attenuation, col);

                        Vec3 exitant_shade = mul_matrix3(world2normal, exitant);
                        Vec3 brdf_vec = brdf_sample(
                                (float)random_sample(&rng),
                                (float)random_sample(&rng),
                                exitant_shade,closure.type,closure.param);
                        //TODO(Vidar): Handle refraction...
                        if(brdf_vec.z > 0.f){
                            float brdf_pdf = brdf_sample_pdf(brdf_vec,
                                exitant_shade,closure.type,closure.param);
                            if(brdf_pdf > EPSILON){
                                embree_set_ray(&ray,p,
                                        mul_matrix3(normal2world, brdf_vec),
                                        (float)EPSILON, FLT_MAX);
                                path_attenuation = scale_vec3(path_attenuation,
                                    (1.f/brdf_pdf) * (max_float(0.f, dot(ray.dir, n)))
                                    * brdf_eval(brdf_vec,exitant_shade,
                                           closure.type,closure.param)
                                    * closure_inv_prob);
                            } else {
                                break;
                            }
                        } else {
                            // NOTE(Vidar): Sample direction below the normal,
                            // sample wasted...
                            break;
                        }

                        osl_free_closures(closures);
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

    osl_release_thread_context(osl_thread_context,shading_system);
}

