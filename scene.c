#include "scene.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define READ_ARRAY(var,type,num,file) var = (type *)malloc(sizeof(type)*num);\
    fread(var,sizeof(type),num,file)

OzyScene* ozy_scene_create_from_file(const char *filename)
{
    OzyScene *scene = malloc(sizeof(OzyScene));
    memset(scene,0,sizeof(OzyScene));
    printf("loading scene \"%s\"\n", filename);

    FILE *f = fopen(filename, "rb");
    if(f){
        //printf("Opened file %s success!\n",filename);
        fread(&(scene->num_tris),sizeof(int),1,f);
        fread(&(scene->num_verts),sizeof(int),1,f);
        fread(&(scene->num_materials),sizeof(int),1,f);

        READ_ARRAY(scene->tris, u32, scene->num_tris*3,f);

        scene->verts = (vec3 *)malloc(sizeof(vec3)*scene->num_verts);
        for(u32 i=0;i<scene->num_verts;i++){
            fread(scene->verts+i,sizeof(float)*3,1,f);
        }

        scene->normals = (vec3 *)malloc(sizeof(vec3)*scene->num_verts);
        for(u32 i=0;i<scene->num_verts;i++){
            fread(scene->normals+i,sizeof(float)*3,1,f);
        }

        READ_ARRAY(scene->tri_material,u32, scene->num_tris,  f);

        Camera cam = {};
        fread(&(cam.pos),      sizeof(float)*3,   1,f);
        fread(&(cam.transform),sizeof(Matrix3),1,f);
        fread(&(cam.fov),      sizeof(float)  ,1,f);

        scene->camera = cam;

        scene->materials =
            (Material*)malloc(sizeof(Material)*scene->num_materials);

        u32 *emit_materials = (u32*)malloc(sizeof(int)*scene->num_materials);
        u32 num_emit_materials = 0;

        for(u32 i=0;i<scene->num_materials;i++){
            Material material = {};
            fread(&(material.color),sizeof(float)*3,1,f);
            fread(&(material.emit), sizeof(float)*3,1,f);
            int brdf_type = 0;
            fread(&(brdf_type),sizeof(int),1,f);
            switch(brdf_type){
                case 0:
                    material.brdf = get_lambert_brdf();
                    break;
                case 1:
                    {
                        float ior, shininess;
                        fread(&ior,sizeof(float),1,f);
                        fread(&shininess, sizeof(float),1,f);
                        material.brdf = get_phong_brdf(ior,shininess);
                    }
                    break;
                default:
                    //TODO(Vidar): use the cool clang features for this...
                    assert(0);
            }
            scene->materials[i] = material;
            if(vec3_max_element(material.emit) > EPSILON){
                emit_materials[num_emit_materials++] = i;
            }
        }

        //TODO(Vidar): there's a bit of wasted space here, need dynamic array
        scene->light_tris = (u32*)malloc(sizeof(u32)*scene->num_tris);

        for(u32 i=0;i<scene->num_tris;i++){
            for(u32 j=0;j<num_emit_materials;j++){
                if(scene->tri_material[i] == emit_materials[j]){
                    scene->light_tris[scene->num_light_tris++] = i;
                }
            }
        }

        free(emit_materials);

        fclose(f);
    } else {
        printf("ERROR: Could not open file %s!\n",filename);
    }
    return scene;
}

void ozy_scene_destroy(OzyScene * scene)
{
    //TODO(Vidar): Free verts etc.
    free(scene->verts);
    free(scene->normals);
    for(u32 i=0;i<scene->num_materials;i++){
        free_brdf(scene->materials[i].brdf);
    }
    free(scene->materials);
    free(scene->light_tris);
    free(scene->tris);
    free(scene->tri_material);
    free(scene);
}

void ozy_scene_set_geometry(OzyScene * scene,
        int num_verts, float *verts, float *normals,
        int num_tris, unsigned *tris, unsigned *tri_material)
{
    scene->num_verts = (u32)num_verts;
    scene->num_tris  = (u32)num_tris;
    scene->verts     = (vec3*)verts;
    scene->normals   = (vec3*)normals;
    scene->tris = tris;
    scene->tri_material = tri_material;
}

/*void ozy_scene_add_material(UNUSED Scene * scene,
        UNUSED int type, UNUSED float r, UNUSED float g,
        UNUSED float b, UNUSED float emit)
{
    //TODO(Vidar): Implement...
}

void ozy_scene_finalize(UNUSED Scene * scene)
{
    //TODO(Vidar): Implement...
}
*/
