#include "ozymandias_public.h"
#include <stdio.h>

typedef struct{
    const char *out_filename;
    OzyShot *shot;
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
                ozy_shot_save_to_file(context->shot,context->out_filename);
                putchar('\n');
            } break;
    }
}

s32 main(UNUSED s32 argc, UNUSED char **argv)
{
    //TODO(Vidar): Read this from the command line instead...
#ifdef _WIN32
    const char *scene_filename = "c:/temp/scene.ozy";
    const char *out_filename  = "c:/temp/ozy_out";
#else
    const char *scene_filename = "/tmp/scene.ozy";
    const char *out_filename  = "/tmp/ozy";
#endif
    OzyScene *scene = ozy_scene_read_scene_file(scene_filename);
    OzyWorkers *workers = ozy_workers_create(8);

    OzyShot *shot = ozy_shot_create(512,512);
    ozy_shot_set_num_buckets(shot,4,4);
    ozy_shot_set_uniform_subsamples(shot,10);
    for(u32 pass = 0; pass < PASS_COUNT; pass++){
        ozy_shot_enable_pass(shot,pass);
    }

    Context context = {};
    context.out_filename = out_filename;
    context.shot = shot;

    ozy_shot_render(shot,scene,workers,&progress_callback,&context);

    ozy_shot_destroy(shot);
    ozy_workers_destroy(workers);
    ozy_scene_destroy(scene);
}

