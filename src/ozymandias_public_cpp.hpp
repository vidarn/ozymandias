#pragma once
extern "C"{
#include "ozymandias_public.h"
}

#include <memory>
namespace ozymandias {
class Scene
{
public:
    Scene(): scene(ozy_scene_create()){}
    inline void destroy(){
        ozy_scene_destroy(scene);
    }
    u32 add_object(u32 num_verts, u32 num_normals, u32 num_uvs, u32 num_tris)
    {
        return ozy_scene_add_object(scene, num_verts, num_normals, num_uvs,
                num_tris);
    }
    void obj_set_verts(u32 obj, Vec3 *verts)
    {
        return ozy_scene_obj_set_verts(scene, obj, verts);
    }
    void obj_set_tris(u32 obj, u32 *tris)
    {
        return ozy_scene_obj_set_tris(scene, obj, tris);
    }
    void obj_set_normals(u32 obj, Vec3 *normals)
    {
        return ozy_scene_obj_set_normals(scene, obj, normals);
    }
    void obj_set_uvs(u32 obj, float *uvs)
    {
        return ozy_scene_obj_set_uvs(scene, obj, uvs);
    }
    void obj_set_tri_materials(u32 obj, u32 *tri_materials)
    {
        return ozy_scene_obj_set_tri_materials(scene, obj, tri_materials);
    }
    void obj_set_tri_normals(u32 obj, u32 *tri_normals)
    {
        return ozy_scene_obj_set_tri_normals(scene, obj, tri_normals);
    }
    void obj_set_tri_uvs(u32 obj, u32 *tri_uvs)
    {
        return ozy_scene_obj_set_tri_uvs(scene, obj, tri_uvs);
    }
    void obj_set_transform(u32 obj, Matrix4 mat)
    {
        return ozy_scene_obj_set_transform(scene, obj, mat);
    }
    u32 add_material(const char *shader)
    {
        return ozy_scene_add_material(scene, shader);
    }
    void material_set_float_param(u32 material, const char *name, float val)
    {
        return ozy_scene_material_set_float_param(scene, material, name, val);
    }
    void material_set_color_param(u32 material, const char *name, Vec3 val)
    {
        return ozy_scene_material_set_color_param(scene, material, name, val);
    }
    void set_camera(Matrix4 transform, float fov)
    {
        return ozy_scene_set_camera(scene, transform, fov);
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
    enum OzyDirectLightSampling direct_light_sampling;
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

class ShaderParameter
{
    public:
    OzyShaderParameter param;
    uint8_t get_basetype()
    {
        return param.basetype;
    }
    uint8_t get_aggregate()
    {
        return param.aggregate;
    }
    uint8_t get_vecsemantics()
    {
        return param.vecsemantics;
    }
    const char *get_name()
    {
        return param.name;
    }
};

class ShaderInfo
{
    public:
    OzyShaderInfo info;
    ShaderInfo(const char *filename)
    {
        info = ozy_get_shader_info(filename);
    }
    ~ShaderInfo()
    {
        ozy_free_shader_info(info);
    }
    ShaderParameter get_param(u32 i)
    {
        ShaderParameter ret;
        ret.param = info.params[i];
        return ret;
    }
    u32 num_params()
    {
        return info.num_params;
    }
    bool is_valid()
    {
        return info.valid == 1;
    }
};

}
