#pragma once
#include "buckets.h"
#include "scene.h"

typedef enum
{
    OZY_PROGRESS_RENDER_BEGIN,
    OZY_PROGRESS_BUCKET_DONE,
    OZY_PROGRESS_RENDER_DONE,
    OZY_PROGRESS_COUNT
} OzyState;

typedef struct {
    unsigned bucket_id;
    unsigned num_buckets;
    unsigned num_done;
    Bucket *bucket;
}BucketDoneMessage;

typedef struct {
    u32 image_width, image_height;
    u32 num_buckets_x, num_buckets_y;
    u32 num_threads;
    u32 subsamples_per_thread;
    char pass_enabled[PASS_COUNT];
    void *callback_data;
    void (*progress_callback)(OzyState,void*,void*);
} OzySettings;

EXPORT void ozy_render(OzySettings settings,Scene ozy_scene);
