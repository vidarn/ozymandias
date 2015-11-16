#include "workers.h"
#include "ozymandias_public.h"
#include <stdlib.h>
#include <string.h>

OzyWorkers *ozy_workers_create(unsigned num_threads)
{
    OzyWorkers *workers = malloc(sizeof(OzyWorkers));
    memset(workers,0,sizeof(OzyWorkers));
    workers->num_threads = num_threads;
    return workers;
}

void ozy_workers_destroy(OzyWorkers *workers)
{
    free(workers);
}

