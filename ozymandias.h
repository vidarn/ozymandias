#pragma once
#include "buckets.h"
#include "scene.h"
#include "common.h"

void ozy_render(OzyScene *scene, OzyShot *shot, OzyWorkers *workers,
        OZY_PROGRESS_CALLBACK progress_callback, void *context);
