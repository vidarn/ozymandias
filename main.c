#include "ozymandias_public.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct{
    const char *out_filename;
    OzyResult *result;
} Context;

static void progress_callback(OzyProgressState state, void *message, void *data)
{
    Context *context = (Context*)data;
    switch(state){
        case OZY_PROGRESS_RENDER_BEGIN:
            {
                printf("Rendering...\n");
            } break;
        case OZY_PROGRESS_BUCKET_DONE:
            {
                OzyProgressBucketDoneMessage *msg = (OzyProgressBucketDoneMessage*)message;
                printf("\r[");
                for(u32 ii=0;ii<msg->num_buckets;ii++){
                    if(ii <= msg->num_done){
                        putchar('#');
                    }else{
                        putchar(' ');
                    }
                }
                putchar(']');
                fflush(stdout);
            } break;
        case OZY_PROGRESS_RENDER_DONE:
            {
                ozy_result_save_to_file(context->result,context->out_filename);
                putchar('\n');
                float *buffer = malloc(512*512*4*sizeof(float));
                ozy_result_get_pass(context->result,PASS_FINAL,buffer);
                free(buffer);
            } break;
    }
}

s32 main(UNUSED s32 argc, UNUSED char **argv)
{
    //TODO(Vidar): Read this from the command line instead...
#ifdef _WIN32
    const char *scene_filename = "c:/temp/scene.ozy";
    const char *out_filename   = "c:/temp/ozy_out";
#else
    const char *scene_filename = "/tmp/scene.ozy";
    const char *out_filename   = "/tmp/ozy";
#endif
    OzyShot shot = {};
    shot.width  = 512;
    shot.height = 512;
    shot.num_buckets_x = 4;
    shot.num_buckets_y = 4;
    shot.subsamples_per_thread = 3;
    for(u32 pass = 0; pass < PASS_COUNT; pass++){
        shot.pass_enabled[pass] = 1;
    }

    OzyScene *scene = ozy_scene_create_from_file(scene_filename);
    OzyResult *result = ozy_result_create();
    OzyWorkers *workers = ozy_workers_create(8);

    Context context = {};
    context.out_filename = out_filename;
    context.result = result;

    ozy_render(result, &shot, scene, workers, &progress_callback, &context);

    ozy_result_destroy(result);
    ozy_scene_destroy(scene);
    ozy_workers_destroy(workers);
}

