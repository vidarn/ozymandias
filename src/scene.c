#include "scene.h"
#include "matrix.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

DYNAMIC_ARRAY_IMP(Object)
DYNAMIC_ARRAY_IMP(Material)
DYNAMIC_ARRAY_IMP(LightTri)

OzyScene* ozy_scene_create()
{
    OzyScene *ret = malloc(sizeof(OzyScene));
    memset(ret,0,sizeof(OzyScene));
    ret->camera.transform = identity_matrix4();
    ret->camera.fov = 0.8f;
    ret->valid = 1;
    //NOTE(Vidar): Add default material
    ozy_scene_add_material(ret,"ozy_default",vec3(0.f,0.f,0.f));
    return ret;
}


#define MALLOC_AND_MEMSET(var,type,num) var = malloc(sizeof(type)*num);\
        memset(var,0,sizeof(type)*num)
//TODO(Vidar): Make this less fixed-function. Just different attributes per
// vertex/tri
//TODO(Vidar): Improve this, don't copy things, keep the pointers until later?
u32 ozy_scene_add_object(OzyScene *scene, u32 num_verts, u32 num_normals,
        u32 num_uvs, u32 num_tris)
{
    Object obj = {};
    obj.num_verts     = num_verts;
    obj.num_normals   = num_normals;
    obj.num_uvs       = num_uvs;
    obj.num_tris      = num_tris;
    MALLOC_AND_MEMSET(obj.verts,         Vec3, num_verts);
    MALLOC_AND_MEMSET(obj.normals,       Vec3, num_normals);
    MALLOC_AND_MEMSET(obj.uvs,           Vec3, num_uvs);
    MALLOC_AND_MEMSET(obj.tris,          u32,  num_tris*3);
    MALLOC_AND_MEMSET(obj.tri_materials, u32,  num_tris);
    MALLOC_AND_MEMSET(obj.tri_normals,   u32,  num_tris*3);
    MALLOC_AND_MEMSET(obj.tri_uvs,       u32,  num_tris*3);
    obj.transform = identity_matrix4();
    u32 id = scene->objects.count;
    da_push_Object(&scene->objects,obj);
    return id;
}

void ozy_scene_obj_set_verts(OzyScene *scene, u32 obj, Vec3 *verts)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->verts,verts,object->num_verts*sizeof(Vec3));
}

void ozy_scene_obj_set_tris(OzyScene *scene, u32 obj, u32 *tris)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->tris,tris,object->num_tris*sizeof(u32)*3);
}

void ozy_scene_obj_set_normals(OzyScene *scene, u32 obj, Vec3 *normals)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->normals,normals,object->num_normals*sizeof(Vec3));
}

void ozy_scene_obj_set_uvs(OzyScene *scene, u32 obj, float *uvs)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->uvs,uvs,object->num_uvs*sizeof(float)*2);
}

void ozy_scene_obj_set_tri_normals(OzyScene *scene, u32 obj, u32 *tri_normals)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->tri_normals,tri_normals,object->num_tris*sizeof(u32)*3);
}

void ozy_scene_obj_set_tri_uvs(OzyScene *scene, u32 obj, u32 *tri_uvs)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->tri_uvs,tri_uvs,object->num_tris*sizeof(u32)*3);
}

void ozy_scene_obj_set_tri_materials(OzyScene *scene, u32 obj,
        u32 *tri_materials)
{
    Object *object = scene->objects.data + obj;
    memcpy(object->tri_materials,tri_materials,object->num_tris*sizeof(u32));
}

void ozy_scene_obj_set_transform(OzyScene *scene, u32 obj, Matrix4 mat)
{
    scene->objects.data[obj].transform = mat;
}

u32 ozy_scene_add_material(OzyScene *scene, const char *shader, Vec3 emit)
{
    Material mat = {};
    mat.shader_name = strdup(shader);
    mat.emit = emit;
    u32 id = scene->materials.count;
    da_push_Material(&scene->materials,mat);
    return id;
}

void ozy_scene_material_set_float_param(OzyScene *scene, u32 material,
        const char *name, float val)
{
    Material* mat = scene->materials.data+material;
    mat->num_params++;
    mat->params = realloc(mat->params,mat->num_params*sizeof(OSL_Parameter*));
    mat->params[mat->num_params-1] = osl_new_float_parameter(name,val);
}

void ozy_scene_material_set_color_param(OzyScene *scene, u32 material,
        const char *name, Vec3 val)
{
    Material* mat = scene->materials.data+material;
    mat->num_params++;
    mat->params = realloc(mat->params,mat->num_params*sizeof(OSL_Parameter*));
    float f[3] = {val.x,val.y,val.z};
    mat->params[mat->num_params-1] = osl_new_color_parameter(name,f);
}

void ozy_scene_set_camera(OzyScene *scene, Matrix4 transform, float fov)
{
    Camera cam = {};
    cam.transform = transform;
    cam.fov = fov;
    scene->camera = cam;
}

void scene_update_light_tris(OzyScene *scene)
{
    u8 *material_emit = malloc(scene->materials.count);
    for(u32 i=0;i<scene->materials.count;i++){
        const Material material = scene->materials.data[i];
        material_emit[i] = vec3_max_element(material.emit) > EPSILON;
    }

    float total_area = 0.f;
    for(u32 i=0;i<scene->objects.count;i++){
        Object *obj = scene->objects.data + i;
        for(u32 ii=0;ii<obj->num_tris;ii++){
            if(material_emit[obj->tri_materials[ii]]){
                Vec3 v1 = obj->verts[obj->tris[ii*3+0]];
                Vec3 v2 = obj->verts[obj->tris[ii*3+1]];
                Vec3 v3 = obj->verts[obj->tris[ii*3+2]];
                float area = magnitude(cross(sub_vec3(v2,v1),sub_vec3(v3,v1)))
                    *0.5f;
                LightTri lt = {i,ii,0.f,0.f,area};
                total_area += area;
                da_push_LightTri(&scene->light_tris,lt);
            }
        }
    }
    //TODO(Vidar): Calculate CDF
    float cumulative_area = 0.f;
    for(u32 i=0;i<scene->light_tris.count;i++){
        cumulative_area += scene->light_tris.data[i].area;
        scene->light_tris.data[i].cdf = cumulative_area/total_area;
        scene->light_tris.data[i].pmf =
            scene->light_tris.data[i].area/total_area;
    }
    free(material_emit);
}

void scene_apply_transforms(OzyScene *scene)
{
    for(u32 i=0;i<scene->objects.count;i++){
        Object *obj = scene->objects.data + i;
        for(u32 ii=0;ii<obj->num_verts;ii++){
            Vec3 vert = obj->verts[ii];
            Vec4 v = vec4(vert.x, vert.y, vert.z, 1.f);
            Vec4 result = mul_matrix4(obj->transform,v);
            obj->verts[ii] = vec3(result.x,result.y,result.z);
        }
        for(u32 ii=0;ii<obj->num_normals;ii++){
            //TODO(Vidar): It should be enough to use the upper left
            // 3x3 block of m
            Matrix4 m = transpose_matrix4(obj->transform);
            Vec3 nor = obj->normals[ii];
            Vec4 n = vec4(nor.x, nor.y, nor.z, 0.f);
            Vec4 result = gauss_matrix4_vec4(m,n);
            //TODO(Vidar):We shouldn't need this normalize...
            obj->normals[ii] = normalize(vec3(result.x,result.y,result.z));
        }
    }
}

void ozy_scene_destroy(OzyScene * scene)
{
    for(u32 i=0;i<scene->objects.count;i++){
        Object *obj = scene->objects.data + i;
        free(obj->verts);
        free(obj->normals);
        free(obj->tris);
        free(obj->tri_materials);
        free(obj->tri_normals);
    }
    da_destroy_Object(&scene->objects);
    for(u32 i=0;i<scene->materials.count;i++){
        Material *mat = scene->materials.data + i;
        for(u32 ii=0;ii<mat->num_params;ii++){
            osl_free_parameter(mat->params[ii]);
        }
        free(mat->params);
        free(mat->shader_name);
    }
    da_destroy_Material(&scene->materials);
    free(scene);
}
