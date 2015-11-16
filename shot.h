#pragma once
#include "common.h"
#include "ozymandias_public.h"
#include "buckets.h"
struct OzyShot
{
    u32 subsamples_per_thread;
    BucketGrid bucket_grid;
};

void ozy_shot_commit(OzyShot *shot);
