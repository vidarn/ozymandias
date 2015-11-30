#include "result.h"
#include "libs/oiio/oiio.h"
#include <stdlib.h>
#include <string.h>

static const char * pass_extension[PASS_COUNT] = {
    "_final",
    "_normal",
    "_color",
    "_depth"
};

static const u8 pass_linear[PASS_COUNT] = {
    0,
    1,
    1,
    1
};

static const char * pass_channel_names[] = {
    "RenderLayer.Combined.R","RenderLayer.Combined.G","RenderLayer.Combined.B","RenderLayer.Combined.A",
    "RenderLayer.Normal.X","RenderLayer.Normal.Y","RenderLayer.Normal.Z",
    "RenderLayer.Color.R", "RenderLayer.Color.G", "RenderLayer.Color.B",
    "RenderLayer.Depth.Z"
};

OzyResult* ozy_result_create()
{
    return malloc(sizeof(OzyResult));
}

void ozy_result_destroy(OzyResult* result)
{
    bucket_grid_destroy(&result->bucket_grid);
    free(result);
}

static void to_sRGB(float *pixel_buffer, u32 num_values)
{
    //TODO(Vidar): Do this properly...
    for(u32 i=0;i<num_values;i++){
        pixel_buffer[i] = powf(min(max(pixel_buffer[i],0.f),1.f),1.f/2.2f);
    }
}

void ozy_result_save_to_file(OzyResult* result, const char* fn,
        const char *format, OzyColorSpace colorspace)
{
    char *filename = malloc(strlen(fn)+40+strlen(format));
    BucketGrid *bucket_grid = &result->bucket_grid;
    const u32 w = bucket_grid->width;
    const u32 h = bucket_grid->height;
    const u32 s = w*h;

    float *pixel_buffer;
    u32 pass_count             = PASS_COUNT;
    const u32 *pass_channels   = ozy_pass_channels;
    u32 *pass_offsets          = bucket_grid->pass_offset;
    u32 *pass_enabled          = bucket_grid->pass_enabled;
    const char *channel_names[pass_count*4];

    strcpy(filename,fn);
    strcat(filename,".");
    strcat(filename,format);
    u8 use_passes = oiio_format_supports_passes(filename);
    u32 max_num_channels = 0;

    u32 _pass_channels = 0;
    u32 _pass_offsets = 0;
    u32 _pass_enabled = 1;
    if(use_passes){
        //NOTE(Vidar): If the format supports passes, we write all the data in 
        // one go
        int channel = 0;
        u32 offset = 0;
        for(u32 pass = 0; pass < pass_count; pass++){
            if(bucket_grid->pass_enabled[pass]){
                for(u32 c = 0; c < pass_channels[pass]; c++){
                    channel_names[channel++] = pass_channel_names[offset+c];
                }
                _pass_channels += pass_channels[pass];
            }
            offset += pass_channels[pass];
        }
        pass_count    = 1;
        pass_channels = &_pass_channels;
        pass_offsets  = &_pass_offsets;
        max_num_channels = _pass_channels;
        pass_enabled  = &_pass_enabled;
    } else {
        for(u32 pass = 0; pass < pass_count; pass++){
            max_num_channels = max(max_num_channels, pass_channels[pass]);
        }
    }
    pixel_buffer  = malloc(s*sizeof(float)*max_num_channels);

    char done_buckets[bucket_grid->num_buckets];
    char active_buckets[bucket_grid->num_buckets];
    memcpy(done_buckets,bucket_grid->done_buckets,bucket_grid->num_buckets);
    memcpy(active_buckets,bucket_grid->active_buckets,bucket_grid->num_buckets);

    for(u32 pass = 0; pass < pass_count;pass++){
        if(pass_enabled[pass]){
            u32 i = 0;

            //TODO(Vidar):Fix this, we want a function that maps from x,y to bucket
            u32 bucket_width  = (bucket_grid->buckets[0].max_x
                    - bucket_grid->buckets[0].min_x);
            u32 bucket_height = (bucket_grid->buckets[0].max_y
                    - bucket_grid->buckets[0].min_y);

            for(u32 y=0;y<h;y++)
            {
                for(u32 x=0;x<w;x++)
                {
                    u32 bucket_x = (x/bucket_width);
                    u32 bucket_y = (y/bucket_height);
                    u32 bucket_id = bucket_x +
                        bucket_y*bucket_grid->num_buckets_x;
                    u32 xx = x - bucket_x*bucket_width;
                    u32 yy = y - bucket_y*bucket_height;
                    if(done_buckets[bucket_id]){
                    //if(0){
                        for(u32 c=0;c<pass_channels[pass];c++)
                        {
                            float a = bucket_grid->buckets[bucket_id].data[
                                (xx+yy*bucket_width) * bucket_grid->pass_stride +
                                pass_offsets[pass] + c];
                            pixel_buffer[i++] = a;
                        }
                    } else {
                        float val = 0.5f;
                        if(active_buckets[bucket_id]){
                            if(xx == 0 || yy == 0 || xx == bucket_width-1 
                                   || yy == bucket_height-1 ){
                                val = 1.f;
                            }
                        }
                        for(u32 c=0;c<pass_channels[pass];c++) {
                            pixel_buffer[i++] = val;
                        }
                    }
                }
            }
            switch(colorspace){
                case OZY_COLORSPACE_LINEAR:
                    break;
                case OZY_COLORSPACE_SRGB:
                    if(!pass_linear[pass]){
                        to_sRGB(pixel_buffer,s*pass_channels[pass]);
                    }
                    break;
            }
            if(use_passes){
                oiio_write_passes_file(filename,w,h,pass_channels[pass],
                        channel_names,pixel_buffer);
            }else{
                strcpy(filename,fn);
                strcat(filename,pass_extension[pass]);
                strcat(filename,".");
                strcat(filename,format);
                oiio_write_pixel_buffer_to_file(filename,w,h,
                        pass_channels[pass],pixel_buffer,colorspace);
            }
        }
    }
    free(pixel_buffer);
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
