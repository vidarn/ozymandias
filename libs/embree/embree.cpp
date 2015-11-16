#include "embree.h"
#include "../../math_common.h"
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <fenv.h>


extern "C"{
    EmbreeScene *embree_init(OzyScene ozy_scene) {
        rtcInit(NULL);
        RTCScene scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
        unsigned geomID = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC ,
                ozy_scene.num_tris, ozy_scene.num_verts);
        float* vertices = (float*) rtcMapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
        for(u32 i=0;i<ozy_scene.num_verts;i++){
            vertices[i * 4 + 0] = ozy_scene.verts[i].x;
            vertices[i * 4 + 1] = ozy_scene.verts[i].y;
            vertices[i * 4 + 2] = ozy_scene.verts[i].z;
        }
        rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
        u32* triangles = (u32*) rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER);
        for(u32 i=0;i<ozy_scene.num_tris*3;i++){
            triangles[i] = ozy_scene.tris[i];
        }
        rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);
        rtcCommit(scene);
        return (EmbreeScene *)scene;
    }

    void embree_set_ray(Ray *ray, vec3 org, vec3 dir, float tnear, float tfar)
    {
        // TODO(Vidar): Do we have to clear anything else?
        // I don't think so...
        ray->org = org;
        ray->dir = dir;
        ray->tnear = tnear;
        ray->tfar  = tfar;
        ray->geomID = RTC_INVALID_GEOMETRY_ID;
        ray->primID = RTC_INVALID_GEOMETRY_ID;
    }

    char embree_occluded(Ray *ray, EmbreeScene *scene)
    {
        DISABLE_FPE; //NOTE(Vidar): Embree throws various FPE's
        rtcOccluded((RTCScene)scene,*(RTCRay*)ray);
        ENABLE_FPE;
        return ray->geomID == 0;
    }

    char embree_intersect(Ray *ray, EmbreeScene *scene)
    {
        DISABLE_FPE;
        rtcIntersect((RTCScene)scene,*(RTCRay*)ray);
        ENABLE_FPE;
        return ray->geomID != (u32)-1;
    }
    void embree_close(EmbreeScene *scene)
    {
        rtcDeleteScene((RTCScene)scene);
        rtcExit();
    }
}
