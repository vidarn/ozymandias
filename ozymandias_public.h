#pragma once
#define PRIVATE
#include "common.h"
#ifdef OZYMANDIAS_INTERNAL
#pragma GCC visibility push(default)
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
    u32 num_buckets_x;
    u32 num_buckets_y;
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
u32 ozy_result_get_num_buckets_x(OzyResult* result);
u32 ozy_result_get_num_buckets_y(OzyResult* result);

OzyWorkers* ozy_workers_create(u32 num_workers);
void ozy_workers_destroy(OzyWorkers *workers);

OzyScene* ozy_scene_create(void);
void ozy_scene_destroy(OzyScene *scene);
OzyScene* ozy_scene_create_from_file(const char* filename);
//TODO(Vidar): replace with better api...
void ozy_scene_set_geometry(OzyScene* scene, int num_verts,float* verts,
        float* normals, int num_tris, u32* tris, u32* tri_material);

#ifdef OZYMANDIAS_INTERNAL
#pragma GCC visibility pop
#endif
