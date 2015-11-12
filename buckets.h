#pragma once
#include "libs/thread/thread.h"
#include "common.h"
#include "vec3.h"

enum
{
    PASS_FINAL,
    PASS_NORMAL,
    PASS_COLOR,
    PASS_DEPTH,
    //---
    PASS_COUNT
};

typedef struct
{
    unsigned min_x, min_y, max_x, max_y;
    unsigned width, height;
    unsigned *num_samples;
    float *data;
}Bucket;

typedef struct {
    //TODO(Vidar): It's not very nice to include the semaphore in the public api
    // Pherhaps we should have an internal field?
    Semaphore bucket_done;
    Bucket *buckets;
    char *done_buckets;
    unsigned num_buckets_x;
    unsigned num_buckets_y;
    unsigned num_buckets;
    unsigned *current_bucket;

    unsigned width; //Size of the final image
    unsigned height;

    char pass_enabled[PASS_COUNT];
    unsigned pass_offset[PASS_COUNT];
    unsigned pass_stride;
} BucketGrid;


//TODO(Vidar): Make this more nice...
#define ADD_PIXEL_SAMPLE_F(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_f(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

#define ADD_PIXEL_SAMPLE_V3(value, pass) if(bucket_grid.pass_enabled[pass]) \
    bucket_add_sample_v3(&bucket,x,y,bucket_grid.pass_stride, \
            bucket_grid.pass_offset[pass],bucket_width, value, pass);

static inline void bucket_add_sample_v3(Bucket *bucket, int x,
        int y, int pass_stride, int pass_offset, int bucket_width, vec3 value,
        int pass)
{
    int a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset+0] += value.x;
    bucket->data[a*pass_stride + pass_offset+1] += value.y;
    bucket->data[a*pass_stride + pass_offset+2] += value.z;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}

static inline void bucket_add_sample_f(Bucket *bucket, int x,
        int y, int pass_stride, int pass_offset, int bucket_width, int value,
        int pass)
{
    int a = (x-bucket->min_x) + (y-bucket->min_y)*bucket_width;
    bucket->data[a*pass_stride + pass_offset] += value;
    bucket->num_samples[a*PASS_COUNT + pass]++;
}

//TODO(Vidar): Implement and use
void bucket_write_to_disk(Bucket bucket, const char *filename);

BucketGrid bucket_grid_create(unsigned num_x, unsigned num_y, unsigned width,
        unsigned height, char *pass_enabled);
void bucket_grid_finalize(BucketGrid bucket_grid);
unsigned bucket_grid_wait_for_next_done(BucketGrid bucket_grid);
void bucket_grid_write_to_disk(BucketGrid *bucket_grid, const char *fn);
