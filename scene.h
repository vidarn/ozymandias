#pragma once
#include "vec3.h"
#include "brdf.h"
#include "matrix.h"
#include "math_common.h"
#include "common.h"

typedef struct 
{
    BRDF brdf;
    vec3 color;
    vec3 emit; // TODO(Vidar): perhaps its better to use a special brdf?
}Material;

typedef struct 
{
    vec3 pos; // TODO(Vidar): Use matrix4 instead?
    float fov;
    Matrix3 transform;
}Camera;

typedef struct 
{
    unsigned num_tris, num_verts, num_materials, num_light_tris;
    vec3 *verts, *normals;
    unsigned *tris, *tri_material, *light_tris;
    Camera camera;
    Material *materials;
}Scene;

EXPORT Scene read_scene_file(const char *filename);
