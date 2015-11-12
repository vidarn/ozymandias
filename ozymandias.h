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
    int image_width, image_height;
    int num_buckets_x, num_buckets_y;
    int num_threads;
    int subsamples_per_thread;
    char pass_enabled[PASS_COUNT];
    void *callback_data;
    void (*progress_callback)(OzyState,void*,void*);
} OzySettings;

void ozy_render(OzySettings settings,Scene ozy_scene);
