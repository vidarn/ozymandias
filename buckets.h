#pragma once
#include "libs/thread/thread.h"
#include "common.h"
#include "ozymandias_public.h"
#include "vec3.h"

typedef struct
{
    u32 min_x, min_y, max_x, max_y;
    u32 width, height;
    u32 *num_samples;
    float *data;
}Bucket;

typedef struct {
    Semaphore bucket_done;
    Bucket *buckets;
    //TODO(Vidar): better with a bitflag per bucket...
    u8 *done_buckets, *handled_buckets, *active_buckets;
    u32 num_buckets_x;
    u32 num_buckets_y;
    u32 num_buckets;
    u32 *current_bucket;

    u32 width; //Size of the final image
    u32 height;

    u32 pass_enabled[PASS_COUNT];
    u32 pass_offset[PASS_COUNT];
    u32 pass_stride;
} BucketGrid;

//TODO(Vidar): Make this more nice...
#define ADD_PIXEL_SAMPLE_1(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_1(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

#define ADD_PIXEL_SAMPLE_3(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_3(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

//TODO(Vidar): The vectors are 4 long, why not use that?
#define ADD_PIXEL_SAMPLE_4(value, alpha, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_4(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, alpha, pass);

static inline void bucket_add_sample_4(Bucket *bucket, u32 x,
        u32 y, u32 pass_stride, u32 pass_offset,
        u32 bucket_width, vec3 value, float alpha, u32 pass)
{
    u32 a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset+0] += value.x;
    bucket->data[a*pass_stride + pass_offset+1] += value.y;
    bucket->data[a*pass_stride + pass_offset+2] += value.z;
    bucket->data[a*pass_stride + pass_offset+3] += alpha;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}

static inline void bucket_add_sample_3(Bucket *bucket, u32 x,
        u32 y, u32 pass_stride, u32 pass_offset,
        u32 bucket_width, vec3 value, u32 pass)
{
    u32 a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset+0] += value.x;
    bucket->data[a*pass_stride + pass_offset+1] += value.y;
    bucket->data[a*pass_stride + pass_offset+2] += value.z;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}

static inline void bucket_add_sample_1(Bucket *bucket, u32 x,
        u32 y, u32 pass_stride, u32 pass_offset,
        u32 bucket_width, float value, u32 pass)
{
    u32 a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset] += value;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}


void bucket_grid_create(BucketGrid *bucket_grid);
void bucket_grid_destroy(BucketGrid *bucket_grid);
void bucket_grid_finalize_bucket(BucketGrid bucket_grid, u32 bucket_id);
u32 bucket_grid_wait_for_next_done(BucketGrid bucket_grid);
EXPORT void bucket_grid_write_to_disk(BucketGrid *bucket_grid, const char *fn);
//TODO(Vidar): Implement and use
//EXPORT void bucket_write_to_disk(Bucket bucket, const char *filename);
