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
    char *done_buckets;
    u32 num_buckets_x;
    u32 num_buckets_y;
    u32 num_buckets;
    u32 *current_bucket;

    u32 width; //Size of the final image
    u32 height;

    char pass_enabled[PASS_COUNT];
    u32 pass_offset[PASS_COUNT];
    u32 pass_stride;
} BucketGrid;

extern const u16 pass_channels[PASS_COUNT];
extern const char * pass_extension[PASS_COUNT];

//TODO(Vidar): Make this more nice...
#define ADD_PIXEL_SAMPLE_F(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_f(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

#define ADD_PIXEL_SAMPLE_V3(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_v3(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

static inline void bucket_add_sample_v3(Bucket *bucket, u32 x,
        u32 y, u32 pass_stride, u32 pass_offset,
        u32 bucket_width, vec3 value, u32 pass)
{
    u32 a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset+0] += value.x;
    bucket->data[a*pass_stride + pass_offset+1] += value.y;
    bucket->data[a*pass_stride + pass_offset+2] += value.z;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}

static inline void bucket_add_sample_f(Bucket *bucket, u32 x,
        u32 y, u32 pass_stride, u32 pass_offset,
        u32 bucket_width, float value, u32 pass)
{
    u32 a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset] += value;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}


void bucket_grid_create(BucketGrid *bucket_grid);
void bucket_grid_finalize(BucketGrid bucket_grid);
u32 bucket_grid_wait_for_next_done(BucketGrid bucket_grid);
EXPORT void bucket_grid_write_to_disk(BucketGrid *bucket_grid, const char *fn);
//TODO(Vidar): Implement and use
//EXPORT void bucket_write_to_disk(Bucket bucket, const char *filename);
