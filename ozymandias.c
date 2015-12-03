//#define _GNU_SOURCE
#include <fenv.h>

#include "ozymandias_public.h"
#include "ozymandias.h"
#include "common.h"
#include "path_tracer.h"
#include "scene.h"
#include "shot.h"
#include "workers.h"
#include "result.h"
// Embree wrapper
#include "libs/embree/embree.h"

#include <string.h>
#include <stdlib.h>

typedef struct 
{
    RenderParams *render_params;
    BucketGrid   *bucket_grid;
    u32 thread_id;
}ThreadParams;


static u32 hilbert_curve_transform_bucket_id(u32 n, u32 d){
    u32 rx, ry;
    u32 x = 0;
    u32 y = 0;
    for (u32 s=1; s<n; s*=2) {
        rx = 1 & (d/2);
        ry = 1 & (d ^ rx);
        if (ry == 0) {
            if (rx == 1) {
                x = s-1 - x;
                y = s-1 - y;
            }
            u32 tmp  = x;
            x = y;
            y = tmp;
        }
        x += s * rx;
        y += s * ry;
        d /= 4;
    }
    return (u32)x + (u32)y*n;
}

static unsigned long thread_func(void *param)
{
    ThreadParams *thread_params = (ThreadParams*)param;
    BucketGrid   *bucket_grid   = (BucketGrid*)thread_params->bucket_grid;
    RenderParams *render_params = (RenderParams*)thread_params->render_params;

    u32 bucket_id = __sync_fetch_and_add(bucket_grid->current_bucket, 1);
    while(bucket_id < bucket_grid->num_buckets){
        u32 bucket_index = hilbert_curve_transform_bucket_id(
                bucket_grid->num_buckets_x,bucket_id);
        bucket_grid->active_buckets[bucket_index] = 1;
        path_trace(*render_params,*bucket_grid,bucket_index);
        bucket_grid->active_buckets[bucket_index] = 0;
        bucket_grid->done_buckets[bucket_index] = 1;
        semaphore_post(bucket_grid->bucket_done);
        bucket_id = __sync_fetch_and_add(bucket_grid->current_bucket, 1);
    }
    return 0;
}


void ozy_render(OzyResult *result, OzyShot *shot, OzyScene *scene,
        OzyWorkers *workers, OZY_PROGRESS_CALLBACK progress_callback,
        void *context)
{
    memset(result,0,sizeof(OzyResult));
    result->bucket_grid.num_buckets_x = 1 << shot->bucket_resolution;
    result->bucket_grid.num_buckets_y = 1 << shot->bucket_resolution;
    result->bucket_grid.width = shot->width;
    result->bucket_grid.height = shot->height;
    memcpy(result->bucket_grid.pass_enabled,shot->pass_enabled,
            sizeof(shot->pass_enabled));
    bucket_grid_create(&result->bucket_grid);
    if(scene->valid){
        //TODO(Vidar):read directly into RTCScene?
        EmbreeScene *embree_scene = embree_init(*scene);

        //NOTE(Vidar): Enable floating point exceptions
        //ENABLE_FPE;

        ThreadHandle threads[workers->num_threads];
        RenderParams render_params[workers->num_threads];
        ThreadParams thread_params[workers->num_threads];
        RNG          rngs[workers->num_threads];
        memset(render_params,0,sizeof(RenderParams)*workers->num_threads);
        memset(thread_params,0,sizeof(ThreadParams)*workers->num_threads);
        memset(rngs         ,0,sizeof(RNG         )*workers->num_threads);

        init_rngs(rngs,workers->num_threads);

        for(u32 i=0;i<workers->num_threads;i++){
            render_params[i].scene          = *scene;
            render_params[i].embree_scene   = embree_scene;
            render_params[i].num_subsamples = shot->subsamples_per_thread;
            render_params[i].rng            = rngs[i];

            thread_params[i].render_params  = &render_params[i];
            thread_params[i].bucket_grid    = &result->bucket_grid;
            thread_params[i].thread_id      = i;

            threads[i] = thread_start(&thread_func,&(thread_params[i]));
        }

        if(progress_callback){
            progress_callback(OZY_PROGRESS_RENDER_BEGIN,0,
                    context);
        }

        BucketGrid *bucket_grid = &result->bucket_grid;
        for(u32 i=0;i<bucket_grid->num_buckets;i++){
            u32 bucket_id = bucket_grid_wait_for_next_done(*bucket_grid);

            bucket_grid_finalize_bucket(*bucket_grid,bucket_id);

            OzyProgressBucketDoneMessage message = {};
            message.bucket_id   = bucket_id;
            message.num_buckets = bucket_grid->num_buckets;
            message.num_done    = i;

            if(progress_callback){
                progress_callback(OZY_PROGRESS_BUCKET_DONE,(void*)&message,
                        context);
            }
        }

        for(u32 i=0;i<workers->num_threads;i++){
            thread_wait(threads[i]);
        }

        embree_close(embree_scene);

        if(progress_callback){
            progress_callback(OZY_PROGRESS_RENDER_DONE,(void*)bucket_grid,
                    context);
        }

    }
}
