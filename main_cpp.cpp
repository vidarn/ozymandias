#include "ozymandias_public_cpp.h"
#include <iostream>

typedef struct{
    const char *out_filename;
    ozymandias::Shot *shot;
} Context;

static void progress_callback(OzyProgressState state, void *message, void *data)
{
    Context *context = (Context*)data;
    switch(state){
        case OZY_PROGRESS_RENDER_BEGIN:
            {
                std::cout << "Rendering...\n";
            } break;
        case OZY_PROGRESS_BUCKET_DONE:
            {
                OzyProgressBucketDoneMessage *msg
                    = (OzyProgressBucketDoneMessage*)message;
                std::cout << "\r[";
                for(u32 ii=0;ii<msg->num_buckets;ii++){
                    if(ii <= msg->num_done){
                        std::cout << '#';
                    }else{
                        std::cout << ' ';
                    }
                }
                std::cout << ']' << std::flush;
            } break;
        case OZY_PROGRESS_RENDER_DONE:
            {
                context->shot->save_to_file(context->out_filename);
                std::cout << '\n';
            } break;
    }
}


int main(UNUSED int argc, UNUSED char **argv)
{
    Context context;
    ozymandias::Scene scene("/tmp/scene.ozy");
    ozymandias::Workers workers(8);
    ozymandias::Shot shot(512,512);
    context.shot = &shot;
    context.out_filename  = "/tmp/ozy";
    shot.render(scene,workers,progress_callback,&context);
}


