#pragma once
#define PRIVATE
#include "common.h"
#include "vec3.h"
#include "matrix.h"

#ifdef OZYMANDIAS_INTERNAL
#pragma GCC visibility push(default)
#else
//#include "math_public.h"
#endif
typedef enum
{
    OZY_PROGRESS_RENDER_BEGIN,
    OZY_PROGRESS_BUCKET_DONE,
    OZY_PROGRESS_RENDER_DONE
} OzyProgressState;
typedef enum 
{
    PASS_FINAL,
    PASS_NORMAL,
    PASS_COLOR,
    PASS_DEPTH,
    //---
    PASS_COUNT
} OzyPass;
typedef enum
{
    OZY_COLORSPACE_LINEAR,
    OZY_COLORSPACE_SRGB
} OzyColorSpace;
extern const u32 ozy_pass_channels[PASS_COUNT];

typedef struct {
    u32 bucket_id;
    u32 num_buckets;
    u32 num_done;
} OzyProgressBucketDoneMessage;


typedef struct OzyScene   OzyScene;
typedef struct OzyWorkers OzyWorkers;
typedef struct OzyResult  OzyResult;

typedef void (*OZY_PROGRESS_CALLBACK)(OzyProgressState,void*,void*);

struct OzyScene;
struct OzyWorkers;
struct OzyResult;

typedef struct {
    u32 subsamples_per_thread;
    u32 width;
    u32 height;
    u32 bucket_resolution;
    u32 pass_enabled[PASS_COUNT];
} OzyShot;


void ozy_render(OzyResult* result, OzyShot* shot, OzyScene* scene,
        OzyWorkers* workers, OZY_PROGRESS_CALLBACK callback, void* context);

OzyResult* ozy_result_create(void);
void ozy_result_destroy(OzyResult* result);
void ozy_result_save_to_file(OzyResult* result, const char* fn,
        const char *format, OzyColorSpace colorspace);
void ozy_result_get_pass(OzyResult* result, OzyPass pass, float* buffer);
u32  ozy_result_get_num_completed_buckets(OzyResult* result);
void ozy_result_get_bucket(OzyResult* result, OzyPass pass,
        u32 bucket_index, float* buffer);
u32  ozy_result_get_width(OzyResult* result);
u32  ozy_result_get_height(OzyResult* result);
u32  ozy_result_get_bucket_width(OzyResult* result, u32 bucket_id);
u32  ozy_result_get_bucket_height(OzyResult* result, u32 bucket_id);
u32  ozy_result_get_num_buckets_x(OzyResult* result);
u32  ozy_result_get_num_buckets_y(OzyResult* result);

OzyWorkers* ozy_workers_create(u32 num_workers);
void ozy_workers_destroy(OzyWorkers *workers);

OzyScene* ozy_scene_create(void);
void ozy_scene_destroy(OzyScene *scene);

u32 ozy_scene_add_object(OzyScene *scene, u32 num_verts, u32 num_normals,
        u32 num_uvs, u32 num_tris);
void ozy_scene_obj_set_verts(OzyScene *scene, u32 obj, Vec3 *verts);
void ozy_scene_obj_set_tris(OzyScene *scene, u32 obj, u32 *tris);
void ozy_scene_obj_set_normals(OzyScene *scene, u32 obj, Vec3 *normals);
void ozy_scene_obj_set_uvs(OzyScene *scene, u32 obj, float *uvs);
void ozy_scene_obj_set_tri_materials(OzyScene *scene, u32 obj,
        u32 *tri_materials);
void ozy_scene_obj_set_tri_normals(OzyScene *scene, u32 obj, u32 *tri_normals);
void ozy_scene_obj_set_tri_uvs(OzyScene *scene, u32 obj, u32 *tri_uvs);
void ozy_scene_obj_set_transform(OzyScene *scene, u32 obj, Matrix4 mat);

u32 ozy_scene_add_material(OzyScene *scene, const char *shader, Vec3 emit);
void ozy_scene_set_camera(OzyScene *scene, Matrix4 transform, float fov);

#ifdef OZYMANDIAS_INTERNAL
#pragma GCC visibility pop
#endif
