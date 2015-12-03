#pragma once
extern "C"{
#include "ozymandias_public.h"
}

#include <memory>
namespace ozymandias {
class Scene
{
public:
    //Scene(): scene(ozy_scene_create())
    //{ }
    Scene(const char* filename):scene(ozy_scene_create_from_file(filename))
    { }
    inline void destroy(){
        ozy_scene_destroy(scene);
    }
    inline void set_geometry(int num_verts, float* verts, float* normals,
            int num_tris, u32* tris, u32* tri_material){
        return ozy_scene_set_geometry(scene, num_verts, verts, normals,
                num_tris, tris, tri_material);
    }
    OzyScene *scene;
};

class Workers
{
public:
    Workers(u32 num_workers): workers(ozy_workers_create(num_workers))
    { }
    inline void destroy(){
        ozy_workers_destroy(workers);
    }

    OzyWorkers* workers;
};

class Result
{
public:
    Result(): result(ozy_result_create())
    { }
    inline void destroy(){
        ozy_result_destroy(result);
    }
    inline void save_to_file(const char* fn, const char *format,
            OzyColorSpace colorspace){
        return ozy_result_save_to_file(result, fn, format, colorspace);
    }
    inline void get_pass(OzyPass pass, float* buffer){
        return ozy_result_get_pass(result, pass, buffer);
    }
    inline u32 get_num_completed_buckets(){
        return ozy_result_get_num_completed_buckets(result);
    }
    inline void get_bucket(OzyPass pass, u32 bucket_index, float* buffer){
        return ozy_result_get_bucket(result, pass, bucket_index, buffer);
    }
    inline u32 get_width(){
        return ozy_result_get_width(result);
    }
    inline u32 get_height(){
        return ozy_result_get_height(result);
    }
    inline u32 get_bucket_width(u32 bucket_id){
        return ozy_result_get_bucket_width(result,bucket_id);
    }
    inline u32 get_bucket_height(u32 bucket_id){
        return ozy_result_get_bucket_height(result,bucket_id);
    }
    inline u32 get_num_buckets_x(){
        return ozy_result_get_num_buckets_x(result);
    }
    inline u32 get_num_buckets_y(){
        return ozy_result_get_num_buckets_y(result);
    }
    OzyResult *result;
};

typedef void (*PROGRESS_CALLBACK)(OzyProgressState,Result,void*,void*);

struct ProgressCallbackData
{
    void *data;
    PROGRESS_CALLBACK callback;
};

class Shot
{
public:
    u32 subsamples_per_thread;
    u32 width;
    u32 height;
    u32 bucket_resolution;
    u32 pass_enabled[PASS_COUNT];
    void enable_pass(u32 pass) {
        pass_enabled[pass] = 1;
    }
    void disable_pass(u32 pass) {
        pass_enabled[pass] = 0;
    }
    u32 get_pass_enabled(u32 pass) {
        return pass_enabled[pass];
    }
};

inline void render(Result result, Shot shot, Scene scene, Workers workers,
        OZY_PROGRESS_CALLBACK callback, void* context)
{
    ozy_render(result.result, reinterpret_cast<OzyShot*>(&shot), scene.scene, workers.workers,
            callback, context);
}

}
