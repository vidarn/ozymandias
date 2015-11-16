extern "C"{
#include "ozymandias_public.h"
}
#include <memory>

namespace ozymandias {
    class Scene
    {
        friend class Shot;
        private:
        std::shared_ptr<OzyScene> scene;
        public:
            Scene():
                scene(ozy_scene_create(),ozy_scene_destroy)
            { }
            Scene(const char *filename):
                scene(ozy_scene_read_scene_file(filename),ozy_scene_destroy)
            { }
    };

    class Workers
    {
        friend class Shot;
        private:
        std::shared_ptr<OzyWorkers> workers;
        public:
            Workers(unsigned num_threads):
                workers(ozy_workers_create(num_threads),ozy_workers_destroy)
            { }
    };

    class Shot
    {
        private:
            std::shared_ptr<OzyShot> shot;
        public:
            Shot(unsigned w, unsigned h):
                shot(ozy_shot_create(w,h),ozy_shot_destroy)
            { }
            void render(Scene scene, Workers workers, OZY_PROGRESS_CALLBACK callback,
                    void *context){
                ozy_shot_render(shot.get(), scene.scene.get(),
                        workers.workers.get(), callback, context);
            }
            void save_to_file(const char *filename) {
                ozy_shot_save_to_file(shot.get(),filename);
            }
    };
}
