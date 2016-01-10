#pragma once
#include "buckets.h"
#include "scene.h"
#include "libs/embree/embree.h"
#include "libs/osl/osl.h"
#include "random.h"
#include "common.h"

typedef struct 
{
    OzyScene scene;
    EmbreeScene *embree_scene;
    OSL_ShadingSystem *shading_system;
    u32 num_subsamples;
    RNG rng;
}RenderParams;

void path_trace(RenderParams params, BucketGrid bucket_grid, unsigned bucket_id);
