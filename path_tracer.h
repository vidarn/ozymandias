#pragma once
#include "buckets.h"
#include "scene.h"
#include "libs/embree/embree.h"
#include "random.h"

typedef struct 
{
    Scene ozy_scene;
    EmbreeScene *scene;
    int num_subsamples;
    RNG rng;
}RenderParams;

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id);
