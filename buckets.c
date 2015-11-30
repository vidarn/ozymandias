#include "buckets.h"
#include <string.h>
#include <stdlib.h>

// TODO(Vidar) specify this together with the passes...
const u32 ozy_pass_channels[PASS_COUNT] = {4,3,3,1};

void bucket_grid_create(BucketGrid *bucket_grid)
{
    // --- SET UP BUCKETS AND PASSES... ---
    //NOTE(Vidar): To play nice with the caches, the passes are stored
    // interleaved, with pixel one of all passes, followed by pixel two of all 
    // passes, etc.
    // pass_offset determines the offset of each pass and pass_stride the
    // distance between pixels of the same pass

    for(int pass = 0; pass < PASS_COUNT; pass++){
        if(bucket_grid->pass_enabled[pass]){
            bucket_grid->pass_offset[pass] = bucket_grid->pass_stride;
            bucket_grid->pass_stride      += ozy_pass_channels[pass];
        }
    }

    bucket_grid->num_buckets   = bucket_grid->num_buckets_x
        *bucket_grid->num_buckets_y;
    bucket_grid->buckets       = malloc(sizeof(Bucket)*bucket_grid->num_buckets);

    //TODO(Vidar): Handle the case when
    // the width of the image is not divisible by the number of buckets...
    u32 bucket_width  = bucket_grid->width/bucket_grid->num_buckets_x;
    u32 bucket_height = bucket_grid->height/bucket_grid->num_buckets_y;
    u32 bucket_size = bucket_width*bucket_height;

    bucket_grid->bucket_done = semaphore_create(0);

    bucket_grid->done_buckets = malloc(sizeof(u8)*bucket_grid->num_buckets);
    memset(bucket_grid->done_buckets,0,bucket_grid->num_buckets);

    bucket_grid->handled_buckets = malloc(sizeof(u8)*bucket_grid->num_buckets);
    memset(bucket_grid->handled_buckets,0,bucket_grid->num_buckets);

    bucket_grid->active_buckets = malloc(sizeof(u8)*bucket_grid->num_buckets);
    memset(bucket_grid->active_buckets,0,bucket_grid->num_buckets);

    bucket_grid->current_bucket = malloc(sizeof(u32));
    *bucket_grid->current_bucket = 0;

    for(u32 y=0;y<bucket_grid->num_buckets_y;y++)
    {
        for(u32 x=0;x<bucket_grid->num_buckets_x;x++)
        {
            u32 i = x + y*bucket_grid->num_buckets_x;
            bucket_grid->buckets[i].min_x = ( x )*bucket_width;
            bucket_grid->buckets[i].max_x = (x+1)*bucket_width;
            bucket_grid->buckets[i].min_y = ( y )*bucket_height;
            bucket_grid->buckets[i].max_y = (y+1)*bucket_height;
            bucket_grid->buckets[i].width = bucket_grid->buckets[i].max_x
                - bucket_grid->buckets[i].min_x;
            bucket_grid->buckets[i].height = bucket_grid->buckets[i].max_y
                - bucket_grid->buckets[i].min_y;
            u32 alloc_size = bucket_size*bucket_grid->pass_stride
                *sizeof(float);
            bucket_grid->buckets[i].data = malloc(alloc_size);
            memset(bucket_grid->buckets[i].data,0,alloc_size);

            alloc_size = bucket_size*PASS_COUNT*sizeof(u32);
            bucket_grid->buckets[i].num_samples = malloc(alloc_size);
            memset(bucket_grid->buckets[i].num_samples,0,alloc_size);
        }
    }
}

void bucket_grid_destroy(BucketGrid *bucket_grid)
{
    semaphore_destroy(bucket_grid->bucket_done);
    free(bucket_grid->done_buckets);
    free(bucket_grid->handled_buckets);
    free(bucket_grid->active_buckets);
    free(bucket_grid->current_bucket);
    for(u32 i=0;i<bucket_grid->num_buckets;i++){
        free(bucket_grid->buckets[i].data);
        free(bucket_grid->buckets[i].num_samples);
    }
    free(bucket_grid->buckets);
}

void bucket_grid_finalize_bucket(BucketGrid bucket_grid, u32 bucket_id)
{
    Bucket bucket = bucket_grid.buckets[bucket_id];
    u32 bucket_size =
          (bucket.max_x - bucket.min_x) * (bucket.max_y - bucket.min_y);
    u32 a = 0;
    for(u32 ii=0;ii<bucket_size;ii++) {
        for(u32 pass=0;pass<PASS_COUNT;pass++){
            if(bucket_grid.pass_enabled[pass]){
                u32 num_samples = bucket.num_samples[ii*PASS_COUNT + pass];
                if(num_samples > 0){
                    float inv_num_samples = 1.f/(float)num_samples;
                    for(u32 iii = 0;iii<ozy_pass_channels[pass];iii++){
                        bucket.data[a++] *= inv_num_samples;
                    }
                } else {
                    a += ozy_pass_channels[pass];
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
        if(bucket_grid.done_buckets[ii] && !bucket_grid.handled_buckets[ii]){
            bucket_id = ii;
            bucket_grid.handled_buckets[ii] = 1;
            break;
        }
    }
    return bucket_id;
}

