#include "ozymandias_public.h"
#include "matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dynamic_array.h"
DYNAMIC_ARRAY_DEF(u32)
DYNAMIC_ARRAY_IMP(u32)
DYNAMIC_ARRAY_DEF(Vec3)
DYNAMIC_ARRAY_IMP(Vec3)

#define WIDTH 1000
#define HEIGHT 1000

typedef struct{
    const char *out_filename;
    OzyResult *result;
} Context;

typedef struct{
    Vec3 *verts, *normals;
    u32 *tris, *tri_normals;
    u32 num_verts, num_normals, num_tris;
}ObjResult;

static char *skip_to_number(char *s)
{
    while((*s  < '0' || *s > '9') && *s != 0 && *s != '\n'){
        s++;
    }
    return s;
}

static char *skip_past_number(char *s)
{
    while(*s  >= '0' && *s <= '9' && *s != 0 && *s != '\n'){
        s++;
    }
    return s;
}

static ObjResult load_obj(const char *filename)
{
    ObjResult ret = {};
    DynArr_Vec3 verts = da_create_Vec3();
    DynArr_Vec3 normals = da_create_Vec3();
    DynArr_u32 tris = da_create_u32();
    DynArr_u32 tri_normals = da_create_u32();
    FILE *f = fopen(filename,"rt");
    if(f){
        fseek(f,0,SEEK_END);
        size_t len = (size_t)ftell(f);
        fseek(f,0,SEEK_SET);
        char *buffer = malloc(len);
        fread(buffer,1,len,f);
        char *current = buffer;
        while(current < buffer + len){
            switch(*current){
                case '#': {
                          while(*current != '\n'){
                              current++;
                          }
                          break;
                      }
                case 'v': {
                    float a,b,c;
                    if(*(current+1) == 'n'){
                        sscanf(current,"vn %f %f %f",&a,&b,&c);
                        Vec3 n = vec3(a,b,c);
                        da_push_Vec3(&normals,n);
                    }else{
                        sscanf(current,"v %f %f %f",&a,&b,&c);
                        Vec3 v = vec3(a,b,c);
                        da_push_Vec3(&verts,v);
                    }
                } break;
                case 'f': {
                    current++;
                    if(*current == ' '){
                        u32 numbers[9];
                        u32 numbers_found = 0;
                        current = skip_to_number(current);
                        while(*current != '\n' && numbers_found < 9){
                            numbers[numbers_found++] = (u32)atoi(current)-1;
                            current = skip_past_number(current);
                            current = skip_to_number(current);
                        }
                        if(numbers_found >= 3){
                            u32 stride = 3;
                            if(numbers_found < 9){
                                stride = 2;
                            }
                            if(numbers_found < 6){
                                stride = 1;
                            }
                            for(u32 i=0;i<3;i++){
                                da_push_u32(&tris,numbers[i*stride]);
                                if(stride > 1){
                                    da_push_u32(&tri_normals,numbers[i*stride+1]);
                                }
                            }
                        }
                    }
                } break;
            }
            current++;
        }
        free(buffer);
        fclose(f);
    }
    ret.num_verts   = verts.count;
    ret.num_normals = normals.count;
    ret.num_tris    = tris.count/3;
    ret.verts       = verts.data;
    ret.normals     = normals.data;
    ret.tris        = tris.data;
    ret.tri_normals = tri_normals.data;
    return ret;
}

static u32 add_obj_to_scene(OzyScene *scene, const char *filename, u32 material)
{
    ObjResult model = load_obj(filename);
    u32 *tri_materials = malloc(model.num_tris*sizeof(u32));
    for(u32 i=0;i<model.num_tris;i++){
        tri_materials[i] = material;
    }

    u32 obj = ozy_scene_add_object(scene,model.num_verts,model.num_normals,
            model.num_tris);
    ozy_scene_obj_set_verts(scene,obj,model.verts);
    ozy_scene_obj_set_normals(scene,obj,model.normals);
    ozy_scene_obj_set_tris(scene,obj,model.tris);
    ozy_scene_obj_set_tri_materials(scene,obj,tri_materials);
    ozy_scene_obj_set_tri_normals(scene,obj,model.tri_normals);
    return obj;
}



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
                OzyProgressBucketDoneMessage *msg =
                    (OzyProgressBucketDoneMessage*)message;
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
                char *buffer = malloc(strlen(context->out_filename)+40);
                sprintf(buffer,"%s_%d",context->out_filename,msg->num_done);
                ozy_result_save_to_file(context->result,buffer,"exr",
                        OZY_COLORSPACE_LINEAR);
                free(buffer);
            } break;
        case OZY_PROGRESS_RENDER_DONE:
            {
                putchar('\n');
                ozy_result_save_to_file(context->result,context->out_filename,
                        "exr",OZY_COLORSPACE_LINEAR);
                printf("Result saved to %s.exr\n",context->out_filename);
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
    const char *out_filename   = "/tmp/ozy";
#endif
    OzyShot shot = {};
    shot.width  = WIDTH;
    shot.height = HEIGHT;
    shot.bucket_resolution = 2;
    shot.subsamples_per_thread = 8;
    for(u32 pass = 0; pass < PASS_COUNT; pass++){
        shot.pass_enabled[pass] = 1;
    }

    OzyResult *result = ozy_result_create();
    OzyWorkers *workers = ozy_workers_create(8);

    OzyScene *scene = ozy_scene_create();

    u32 mtl_red = ozy_scene_add_lambert_material(scene,vec3(1.f,0.f,0.f),
            vec3(0.f,0.f,0.f));
    u32 mtl_gray = ozy_scene_add_lambert_material(scene,vec3(0.4f,0.4f,0.4f),
            vec3(0.f,0.f,0.f));
    u32 mtl_emit = ozy_scene_add_lambert_material(scene,vec3(0.f,0.f,0.f),
            vec3(1.f,1.f,1.f));

    add_obj_to_scene(scene,"teapot.obj",mtl_red);
    u32 sphere = add_obj_to_scene(scene,"sphere.obj",mtl_gray);
    add_obj_to_scene(scene,"plane.obj", mtl_emit);

    Matrix4 mat = identity_matrix4();
    mat = mul_mat4_mat4(mat,translation_matrix4(vec3(0.f,4.f,0.f)));
    mat = mul_mat4_mat4(mat,scale_matrix4(vec3(0.4f,0.4f,0.4f)));
    mat = mul_mat4_mat4(mat,euler_xyz_rotation_matrix4(vec3(0.f,0.f,2.f)));
    ozy_scene_obj_set_transform(scene,sphere,mat);

    Matrix4 cam_mat = identity_matrix4();
    cam_mat = mul_mat4_mat4(cam_mat,translation_matrix4(vec3(0.f,5.f,-10.f)));
    cam_mat = mul_mat4_mat4(cam_mat,
            euler_xyz_rotation_matrix4(vec3(-0.2f,0.f,0.f)));
    ozy_scene_set_camera(scene,cam_mat,0.85f);

    Context context = {};
    context.out_filename = out_filename;
    context.result = result;

    ozy_render(result, &shot, scene, workers, &progress_callback, &context);

    ozy_result_destroy(result);
    ozy_scene_destroy(scene);
    ozy_workers_destroy(workers);
}

