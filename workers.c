#include "workers.h"
#include "ozymandias_public.h"
#include <stdlib.h>
#include <string.h>

OzyWorkers *ozy_workers_create(u32 num_workers)
{
    OzyWorkers *workers = malloc(sizeof(OzyWorkers));
    workers->num_threads = num_workers;
    return workers;
}

void ozy_workers_destroy(OzyWorkers *workers)
{
    free(workers);
}
