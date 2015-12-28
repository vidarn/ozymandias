#include "ozymandias_public_cpp.hpp"
#include <iostream>
#include <cstring>

typedef struct{
    const char *out_filename;
    ozymandias::Result *result;
} Context;

static void progress_callback(OzyProgressState state, void *message, void *data)
{
    Context *context = static_cast<Context*>(data);
    switch(state){
        case OZY_PROGRESS_RENDER_BEGIN:
            {
                std::cout << "Rendering...\n";
            } break;
        case OZY_PROGRESS_BUCKET_DONE:
            {
                OzyProgressBucketDoneMessage *msg
                    = static_cast<OzyProgressBucketDoneMessage*>(message);
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
                std::cout << "\nSaving to file:" << context->out_filename
                    << std::endl;
                context->result->save_to_file(context->out_filename,"exr",
                        OZY_COLORSPACE_LINEAR);
            } break;
    }
}


int main(UNUSED int argc, UNUSED char **argv)
{
    std::cout << "Ozymandias CPP\n";
    ozymandias::Scene scene;
    ozymandias::Workers workers(8);
    ozymandias::Shot shot;
    shot.width  = 512;
    shot.height = 512;
    shot.bucket_resolution = 2;
    shot.subsamples_per_thread = 10;
    shot.enable_pass(PASS_FINAL);
    shot.enable_pass(PASS_NORMAL);

    Context context;
    ozymandias::Result result;
    context.out_filename  = "/tmp/cpp_ozy";
    context.result = &result;
    ozymandias::render(result,shot,scene,workers,progress_callback,&context);
    workers.destroy();
    scene.destroy();
    result.destroy();
}


