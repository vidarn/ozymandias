#pragma once
#include "vec3.h"
#include "brdf.h"
#include "matrix.h"
#include "math_common.h"
#include "common.h"
#include "ozymandias_public.h"
#include "dynamic_array.h"

typedef struct 
{
    BRDF brdf;
    Vec3 color; //TODO(Vidar): Should these be part of the BRDF instead?
    Vec3 emit; // TODO(Vidar): perhaps we should have a special emissive brdf?
}Material;

typedef struct 
{
    Matrix4 transform;
    float fov;
}Camera;

typedef struct
{
    Vec3 *verts, *normals;
    u32 *tris, *tri_materials, *tri_normals;
    u32 num_tris, num_verts, num_normals;
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
