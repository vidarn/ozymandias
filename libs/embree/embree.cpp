#include "embree.h"
#include "../../math_common.h"
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <fenv.h>

struct EmbreeScene
{
    RTCDevice dev;
    RTCScene scene;
};

extern "C"{
    EmbreeScene *embree_init(OzyScene ozy_scene) {
        RTCDevice dev = rtcNewDevice(NULL);
        RTCScene scene = rtcDeviceNewScene(dev,RTC_SCENE_STATIC, RTC_INTERSECT1);
        for(unsigned i=0;i<ozy_scene.objects.count;i++){
            Object *obj = ozy_scene.objects.data + i;
            unsigned geomID = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC ,
                    obj->num_tris, obj->num_verts);
            float* vertices = static_cast<float*>(rtcMapBuffer(scene, geomID,
                        RTC_VERTEX_BUFFER));
            for(u32 ii=0;ii<obj->num_verts;ii++){
                vertices[ii * 4 + 0] = obj->verts[ii].x;
                vertices[ii * 4 + 1] = obj->verts[ii].y;
                vertices[ii * 4 + 2] = obj->verts[ii].z;
            }
            rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
            u32* triangles = static_cast<u32*>( rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER));
            for(u32 ii=0;ii<obj->num_tris*3;ii++){
                triangles[ii] = obj->tris[ii];
            }
            rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);
        }
        rtcCommit(scene);
        EmbreeScene *embree_scene = new EmbreeScene;
        embree_scene->dev = dev;
        embree_scene->scene = scene;
        return embree_scene;
    }

    void embree_set_ray(Ray *ray, Vec3 org, Vec3 dir, float tnear, float tfar)
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
        rtcOccluded(scene->scene,*reinterpret_cast<RTCRay*>(ray));
        ENABLE_FPE;
        return ray->geomID == 0;
    }

    char embree_intersect(Ray *ray, EmbreeScene *scene)
    {
        DISABLE_FPE;
        rtcIntersect(scene->scene,*reinterpret_cast<RTCRay*>(ray));
        ENABLE_FPE;
        return ray->geomID != static_cast<u32>(-1);
    }
    void embree_close(EmbreeScene *scene)
    {
        rtcDeleteScene(scene->scene);
        rtcDeleteDevice(scene->dev);
        delete scene;
    }
}
