#include "ozymandias.h"
#include "path_tracer.h"
// Embree wrapper
#include "libs/embree/embree.h"

#include <string.h>

#define __USE_GNU
#include <fenv.h>

typedef struct 
{
    RenderParams *render_params;
    BucketGrid   *bucket_grid;
    int thread_id;
}ThreadParams;

unsigned long thread_func(void *param)
{
    ThreadParams *thread_params = (ThreadParams*)param;
    BucketGrid   *bucket_grid   = (BucketGrid*)thread_params->bucket_grid;
    RenderParams *render_params = (RenderParams*)thread_params->render_params;

    int bucket_id = __sync_fetch_and_add(bucket_grid->current_bucket, 1);
    while(bucket_id < bucket_grid->num_buckets){
        path_trace(*render_params,*bucket_grid,bucket_id);
        bucket_grid->done_buckets[bucket_id] = 1;
        semaphore_post(bucket_grid->bucket_done);
        bucket_id = __sync_fetch_and_add(bucket_grid->current_bucket, 1);
    }
    return 0;
}


void ozy_render(OzySettings settings,Scene ozy_scene)
{
    //TODO(Vidar):read directly into RTCScene?
    EmbreeScene *scene = embree_init(ozy_scene);

    //NOTE(Vidar): Enable floating point exceptions
    ENABLE_FPE;

    ThreadHandle threads[settings.num_threads];
    RenderParams render_params[settings.num_threads];
    ThreadParams thread_params[settings.num_threads];
    RNG          rngs[settings.num_threads];
    memset(render_params,0,sizeof(RenderParams)*settings.num_threads);
    memset(thread_params,0,sizeof(ThreadParams)*settings.num_threads);
    memset(rngs         ,0,sizeof(RNG         )*settings.num_threads);

    init_rngs(rngs,settings.num_threads);

    BucketGrid bucket_grid = bucket_grid_create(settings.num_buckets_x,
            settings.num_buckets_y, settings.image_width,
            settings.image_height, settings.pass_enabled);

    for(int i=0;i<settings.num_threads;i++){
        render_params[i].ozy_scene      = ozy_scene;
        render_params[i].scene          = scene;
        render_params[i].num_subsamples = settings.subsamples_per_thread;
        render_params[i].rng            = rngs[i];

        thread_params[i].render_params  = &render_params[i];
        thread_params[i].bucket_grid    = &bucket_grid;
        thread_params[i].thread_id      = i;

        threads[i] = thread_start(&thread_func,&(thread_params[i]));
    }

    settings.progress_callback(OZY_PROGRESS_RENDER_BEGIN,0,settings.callback_data);

    for(int i=0;i<bucket_grid.num_buckets;i++){
        unsigned bucket_id = bucket_grid_wait_for_next_done(bucket_grid);

        BucketDoneMessage message = {};
        message.bucket_id   = bucket_id;
        message.num_buckets = bucket_grid.num_buckets;
        message.num_done    = i;
        message.bucket      = bucket_grid.buckets + bucket_id;

        settings.progress_callback(OZY_PROGRESS_BUCKET_DONE,(void*)&message,
                settings.callback_data);
    }

    embree_close(scene);
    bucket_grid_finalize(bucket_grid);

    settings.progress_callback(OZY_PROGRESS_RENDER_DONE,(void*)&bucket_grid,settings.callback_data);

    //TODO(Vidar): free stuff...

    printf("Done!\n");
}
