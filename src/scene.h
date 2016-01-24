#pragma once
#include "vec3.h"
#include "matrix.h"
#include "math_common.h"
#include "common.h"
#include "ozymandias_public.h"
#include "dynamic_array.h"
#include "libs/osl/osl.h"

typedef struct 
{
    char *shader_name;
    Vec3 emit; // TODO(Vidar): Use emissive BRDF instead
    OSL_Parameter **params;
    u32 num_params;
    //TODO(Vidar): shader parameters
}Material;

typedef struct 
{
    Matrix4 transform;
    float fov;
}Camera;

typedef struct
{
    float *uvs;
    Vec3 *verts, *normals;
    u32 *tris, *tri_materials, *tri_normals, *tri_uvs;
    u32 num_tris, num_verts, num_normals, num_uvs;
    Matrix4 transform;
}Object;

typedef struct
{
    u32 obj, tri;
}LightTri;

DYNAMIC_ARRAY_DEF(Object)
DYNAMIC_ARRAY_DEF(Material)
DYNAMIC_ARRAY_DEF(LightTri)

struct OzyScene{
    Camera camera;
    DynArr_Material materials;
    DynArr_Object objects;
    DynArr_LightTri light_tris;
    u8 valid;
};

void scene_update_light_tris(OzyScene *scene);
void scene_apply_transforms(OzyScene *scene);