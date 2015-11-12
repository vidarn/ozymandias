#pragma once
#include "buckets.h"
#include "scene.h"
#include "libs/embree/embree.h"
#include "random.h"
#include "common.h"

typedef struct 
{
    Scene ozy_scene;
    EmbreeScene *scene;
    u32 num_subsamples;
    RNG rng;
}RenderParams;

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id);
