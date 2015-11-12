#include "buckets.h"
#include <string.h>
#include <stdlib.h>

// TODO(Vidar) specify this together with the passes...
static const u16 pass_channels[PASS_COUNT] = {3,3,3,1};
static const char *pass_extension[PASS_COUNT] = {
    "_final",
    "_normal",
    "_color",
    "_depth",
};

BucketGrid bucket_grid_create(u32 num_x, u32 num_y, u32 width,
        u32 height, char *pass_enabled)
{
    // --- SET UP BUCKETS AND PASSES... ---
    //NOTE(Vidar): To play nice with the caches, the passes are stored
    // interleaved, with pixel one of all passes, followed by pixel two of all 
    // passes, etc.
    // pass_offset determines the offset of each pass and pass_stride the
    // distance between pixels of the same pass

    BucketGrid bucket_grid = {};

    memcpy(bucket_grid.pass_enabled, pass_enabled, sizeof(bucket_grid.pass_enabled));

    for(int pass = 0; pass < PASS_COUNT; pass++){
        if(bucket_grid.pass_enabled[pass]){
            bucket_grid.pass_offset[pass] = bucket_grid.pass_stride;
            bucket_grid.pass_stride      += pass_channels[pass];
        }
    }

    bucket_grid.num_buckets_x = num_x;
    bucket_grid.num_buckets_y = num_y;
    bucket_grid.width  = width;
    bucket_grid.height = height;
    bucket_grid.num_buckets   = bucket_grid.num_buckets_x
        *bucket_grid.num_buckets_y;
    bucket_grid.buckets       = malloc(sizeof(Bucket)*bucket_grid.num_buckets);

    //TODO(Vidar): Handle the case when
    // the width of the image is not divisible by the number of buckets...
    u32 bucket_width  = bucket_grid.width/bucket_grid.num_buckets_x;
    u32 bucket_height = bucket_grid.height/bucket_grid.num_buckets_y;
    u32 bucket_size = bucket_width*bucket_height;

    bucket_grid.bucket_done = semaphore_create(0);
    bucket_grid.done_buckets = malloc(sizeof(u32)*bucket_grid.num_buckets);
    memset(bucket_grid.done_buckets,0,bucket_grid.num_buckets);
    bucket_grid.current_bucket = malloc(sizeof(u32));
    *bucket_grid.current_bucket = 0;

    for(u32 y=0;y<bucket_grid.num_buckets_y;y++)
    {
        for(u32 x=0;x<bucket_grid.num_buckets_x;x++)
        {
            u32 i = x + y*bucket_grid.num_buckets_x;
            bucket_grid.buckets[i].min_x = ( x )*bucket_width;
            bucket_grid.buckets[i].max_x = (x+1)*bucket_width;
            bucket_grid.buckets[i].min_y = ( y )*bucket_height;
            bucket_grid.buckets[i].max_y = (y+1)*bucket_height;
            bucket_grid.buckets[i].width = bucket_grid.buckets[i].max_x
                - bucket_grid.buckets[i].min_x;
            bucket_grid.buckets[i].height = bucket_grid.buckets[i].max_y
                - bucket_grid.buckets[i].min_y;
            u32 alloc_size = bucket_size*bucket_grid.pass_stride
                *sizeof(float);
            bucket_grid.buckets[i].data = malloc(alloc_size);
            memset(bucket_grid.buckets[i].data,0,alloc_size);

            alloc_size = bucket_size*PASS_COUNT*sizeof(u32);
            bucket_grid.buckets[i].num_samples = malloc(alloc_size);
            memset(bucket_grid.buckets[i].num_samples,0,alloc_size);
        }
    }
    return bucket_grid;
}

void bucket_grid_finalize(BucketGrid bucket_grid)
{
    for(u32 i=0;i<bucket_grid.num_buckets;i++) {
        u32 bucket_size =
              (bucket_grid.buckets[i].max_x - bucket_grid.buckets[i].min_x)
            * (bucket_grid.buckets[i].max_y - bucket_grid.buckets[i].min_y);
        u32 a = 0;
        for(u32 ii=0;ii<bucket_size;ii++) {
            for(u32 pass=0;pass<PASS_COUNT;pass++){
                if(bucket_grid.pass_enabled[pass]){
                    u32 num_samples = bucket_grid.buckets[i]
                        .num_samples[ii*PASS_COUNT + pass];
                    if(num_samples > 0){
                        float inv_num_samples = 1.f/(float)num_samples;
                        for(u32 iii = 0;iii<pass_channels[pass];iii++){
                            bucket_grid.buckets[i].data[a++] *= inv_num_samples;
                        }
                    } else {
                        a += pass_channels[pass];
                    }
                }
            }
        }
    }
}

u32 bucket_grid_wait_for_next_done(BucketGrid bucket_grid)
{
    semaphore_wait(bucket_grid.bucket_done);
    u32 bucket_id = 0;
    for(u32 ii=0;ii<bucket_grid.num_buckets;ii++){
        if(bucket_grid.done_buckets[ii]){
            bucket_id = ii;
            bucket_grid.done_buckets[ii] = 0;
            break;
        }
    }
    return bucket_id;
}

void bucket_grid_write_to_disk(BucketGrid *bucket_grid, const char *fn)
{
    char *filename = malloc(strlen(fn)+40);
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

