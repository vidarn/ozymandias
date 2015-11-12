#include "ozymandias.h"

typedef struct{
    const char *out_filename;
} Context;

static void progress_callback(OzyState state, void *message, void *data)
{
    Context *context = (Context*)data;
    switch(state){
        case OZY_PROGRESS_RENDER_BEGIN:
            {
            printf("Rendering...\n");
            } break;
        case OZY_PROGRESS_BUCKET_DONE:
            {
                BucketDoneMessage *msg = (BucketDoneMessage*)message;
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
                BucketGrid *bucket_grid = (BucketGrid*)message;
                putchar('\n');
                bucket_grid_write_to_disk(bucket_grid,context->out_filename);
            } break;
        default:
            break;
    }
}

int main(UNUSED int argc, UNUSED char **argv)
{
    //TODO(Vidar): Read this from the command line instead...
#ifdef _WIN32
    const char *scene_filename = "c:/temp/scene.ozy";
    const char *out_filename  = "c:/temp/ozy_out";
#else
    const char *scene_filename = "/tmp/scene.ozy";
    const char *out_filename  = "/tmp/ozy";
#endif

    Context context = {};
    context.out_filename = out_filename;

    OzySettings settings = {};
    settings.image_width = 512;
    settings.image_height = 512;
    settings.num_buckets_x = 8;
    settings.num_buckets_y = 4;
    settings.num_threads = 8;
    settings.subsamples_per_thread = 10;
    settings.progress_callback = &progress_callback;
    settings.callback_data = &context;
    for(int pass = 0; pass < PASS_COUNT; pass++){
        settings.pass_enabled[pass] = 1;
    }

    Scene scene = read_scene_file(scene_filename);

    ozy_render(settings,scene);
}

