#include "result.h"
#include <stdlib.h>
#include <string.h>

OzyResult* ozy_result_create()
{
    return malloc(sizeof(OzyResult));
}

void ozy_result_destroy(OzyResult* result)
{
    bucket_grid_destroy(&result->bucket_grid);
    free(result);
}

void ozy_result_save_to_file(OzyResult* result, const char* fn)
{
    char *filename = malloc(strlen(fn)+40);
    BucketGrid *bucket_grid = &result->bucket_grid;
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
                    for(u32 c=0;c<ozy_pass_channels[pass];c++)
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
    free(filename);
}


//NOTE(Vidar): The buffer must be width*height*ozy_pass_channels[pass] float values.
void ozy_result_get_pass(OzyResult* result, OzyPass pass, float* buffer)
{
    BucketGrid *bucket_grid = &result->bucket_grid;

    //NOTE(Vidar): We cache this in case any bucket finishes while we're
    // writing the data
    char done_buckets[bucket_grid->num_buckets];
    char active_buckets[bucket_grid->num_buckets];
    memcpy(done_buckets,bucket_grid->done_buckets,bucket_grid->num_buckets);
    memcpy(active_buckets,bucket_grid->active_buckets,bucket_grid->num_buckets);

    if(bucket_grid->pass_enabled[pass]){
        //TODO(Vidar):Fix this, we want a function that maps from x,y to bucket
        u32 bucket_width  = (bucket_grid->buckets[0].max_x - bucket_grid->buckets[0].min_x);
        u32 bucket_height = (bucket_grid->buckets[0].max_y - bucket_grid->buckets[0].min_y);
        
        u32 i =0;
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
                if(done_buckets[bucket_id]){
                    for(u32 c=0;c<ozy_pass_channels[pass];c++)
                    {
                        buffer[i++] = bucket_grid->buckets[bucket_id].data[
                            (xx+yy*bucket_width) * bucket_grid->pass_stride +
                            bucket_grid->pass_offset[pass] + c];
                    }
                } else {
                    float val = 0.f;
                    if(active_buckets[bucket_id]){
                        if(xx == 0 || yy == 0 || xx == bucket_width-1 
                               || yy == bucket_height-1 ){
                            val = 1.f;
                        }
                    }
                    for(u32 c=0;c<ozy_pass_channels[pass];c++) {
                        buffer[i++] = val;
                    }
                }
            }
        }
    }
}

void ozy_result_get_bucket(OzyResult* result, OzyPass pass,
        u32 bucket_index, float* buffer)
{
    BucketGrid *bucket_grid = &result->bucket_grid;
    Bucket *bucket = &bucket_grid->buckets[bucket_index];
    if(bucket_grid->pass_enabled[pass]){
        u32 bucket_width  = (bucket->max_x - bucket->min_x);
        u32 bucket_height = (bucket->max_y - bucket->min_y);
        
        u32 i = 0;
        for(u32 j=0;j<bucket_width*bucket_height;j++){
            for(u32 c=0;c<ozy_pass_channels[pass];c++)
            {
                buffer[i++] = bucket->data[j * bucket_grid->pass_stride +
                    bucket_grid->pass_offset[pass] + c];
            }
        }
    }
}

u32  ozy_result_get_num_completed_buckets(OzyResult* result)
{
    u32 num_done = 0;
    for(u32 i =0;i<result->bucket_grid.num_buckets;i++) {
        if(result->bucket_grid.done_buckets[i]){
            num_done++;
        }
    }
    return num_done;
}

u32  ozy_result_get_width(OzyResult* result)
{
    return result->bucket_grid.width;
}
u32  ozy_result_get_height(OzyResult* result)
{
    return result->bucket_grid.height;
}

u32  ozy_result_get_bucket_width(OzyResult* result, u32 bucket_id)
{
    return result->bucket_grid.buckets[bucket_id].width;
}
u32  ozy_result_get_bucket_height(OzyResult* result, u32 bucket_id)
{
    return result->bucket_grid.buckets[bucket_id].height;
}

u32 ozy_result_get_num_buckets_x(OzyResult* result){
    return result->bucket_grid.num_buckets_x;
}
u32 ozy_result_get_num_buckets_y(OzyResult* result){
    return result->bucket_grid.num_buckets_y;
}
