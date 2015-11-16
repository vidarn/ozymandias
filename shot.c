#include "shot.h"
#include "ozymandias_public.h"
#include "ozymandias.h"
#include <stdlib.h>
#include <string.h>

OzyShot *ozy_shot_create(unsigned w, unsigned h)
{
    OzyShot *shot = malloc(sizeof(OzyShot));
    memset(shot,0,sizeof(OzyShot));
    //NOTE(Vidar): Default settings...
    shot->bucket_grid.width = w;
    shot->bucket_grid.height = h;
    shot->bucket_grid.num_buckets_x = 4;
    shot->bucket_grid.num_buckets_y = 4;
    shot->subsamples_per_thread = 6;
    shot->bucket_grid.pass_enabled[PASS_FINAL] = 1;
    return shot;
}

EXPORT void ozy_shot_render(OzyShot *shot, OzyScene *scene, OzyWorkers *workers,
        OZY_PROGRESS_CALLBACK callback, void *context)
{
    bucket_grid_create(&shot->bucket_grid);
    ozy_render(scene,shot,workers,callback,context);
}

void ozy_shot_destroy(OzyShot *shot)
{
    free(shot);
}

void ozy_shot_set_resolution( OzyShot *shot, unsigned w, unsigned h)
{
    shot->bucket_grid.width  = w;
    shot->bucket_grid.height = h;
}

void ozy_shot_set_num_buckets(OzyShot *shot, unsigned num_x, unsigned num_y)
{
    shot->bucket_grid.num_buckets_x = num_x;
    shot->bucket_grid.num_buckets_y = num_y;
}

void ozy_shot_enable_pass(OzyShot *shot, OzyPass pass)
{
    ASSERT(pass < PASS_COUNT);
    shot->bucket_grid.pass_enabled[pass] = 1;
}

void ozy_shot_set_uniform_subsamples( OzyShot *shot, unsigned subsamples)
{
    shot->subsamples_per_thread = subsamples;
}


void ozy_shot_save_to_file(OzyShot *shot, const char *fn)
{
    char *filename = malloc(strlen(fn)+40);
    BucketGrid *bucket_grid = &shot->bucket_grid;
    for(u32 pass = 0; pass < PASS_COUNT;pass++){
        if(bucket_grid->pass_enabled[pass]){
            strcpy(filename,fn);
            strcat(filename,pass_extension[pass]);
            FILE *f = fopen(filename,"wb");

            //TODO(Vidar):Fix this, we want a function that maps from x,y to bucket
            u32 bucket_width  = (bucket_grid->buckets[0].max_x - bucket_grid->buckets[0].min_x);
            u32 bucket_height = (bucket_grid->buckets[0].max_y - bucket_grid->buckets[0].min_y);

            for(u32 y=0;y<bucket_grid->height;y++)
            {
                for(u32 x=0;x<bucket_grid->width;x++)
                {
                    u32 bucket_x = (x/bucket_width);
                    u32 bucket_y = (y/bucket_height);
                    u32 bucket_id = bucket_x +
                        bucket_y*bucket_grid->num_buckets_x;
                    u32 xx = x - bucket_x*bucket_width;
                    u32 yy = y - bucket_y*bucket_height;
                    for(u32 c=0;c<pass_channels[pass];c++)
                    {
                        float a = bucket_grid->buckets[bucket_id].data[
                            (xx+yy*bucket_width) * bucket_grid->pass_stride +
                            bucket_grid->pass_offset[pass] + c];
                        fwrite(&a,sizeof(float),1,f);
                    }
                }
            }
            fclose(f);
        }
    }
}
