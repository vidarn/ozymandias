#pragma once
#include "common.h"
typedef enum
{
    OZY_PROGRESS_RENDER_BEGIN,
    OZY_PROGRESS_BUCKET_DONE,
    OZY_PROGRESS_RENDER_DONE
} OzyProgressState;

typedef struct {
    unsigned bucket_id;
    unsigned num_buckets;
    unsigned num_done;
}OzyProgressBucketDoneMessage;

typedef void (*OZY_PROGRESS_CALLBACK)(OzyProgressState,void*,void*);

struct OzyScene;
typedef struct OzyScene OzyScene;
EXPORT OzyScene *ozy_scene_create(void);
EXPORT void ozy_scene_destroy(OzyScene * scene);
EXPORT void ozy_scene_set_geometry(OzyScene * scene,
        int num_verts, float *verts, float *normals,
        int num_tris, unsigned *tris, unsigned *tri_material);
EXPORT void ozy_scene_add_material(OzyScene * scene,
        int type, float r, float g, float b, float emit);
EXPORT void ozy_scene_finalize(OzyScene * scene);
EXPORT OzyScene *ozy_scene_read_scene_file(const char *filename);

typedef enum 
{
    PASS_FINAL,
    PASS_NORMAL,
    PASS_COLOR,
    PASS_DEPTH,
    //---
    PASS_COUNT
}OzyPass;

struct OzyWorkers;
typedef struct OzyWorkers OzyWorkers;
EXPORT OzyWorkers *ozy_workers_create(unsigned num_threads);
EXPORT void ozy_workers_destroy(OzyWorkers *workers);

struct OzyShot;
typedef struct OzyShot OzyShot;
EXPORT OzyShot *ozy_shot_create(unsigned w, unsigned h);
EXPORT void ozy_shot_destroy(OzyShot *shot);
EXPORT void ozy_shot_set_resolution( OzyShot *shot, unsigned w,     unsigned h);
EXPORT void ozy_shot_set_num_buckets(OzyShot *shot, unsigned num_x, unsigned num_y);
EXPORT void ozy_shot_enable_pass(OzyShot *shot, OzyPass pass);
EXPORT void ozy_shot_set_uniform_subsamples( OzyShot *shot, unsigned subsamples);
EXPORT void ozy_shot_save_to_file(   OzyShot *shot, const char *filename);
EXPORT void ozy_shot_render(OzyShot *shot, OzyScene *scene, OzyWorkers *workers,
        OZY_PROGRESS_CALLBACK callback, void *context);


