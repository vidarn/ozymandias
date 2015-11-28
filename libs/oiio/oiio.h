#pragma once
#include "../../ozymandias_public.h"

#ifdef __cplusplus 
extern "C" {
#endif
u8 oiio_format_supports_passes(const char *filename);
void oiio_write_passes_file(const char *filename, u32 width, u32 height,
        u32 channels, const char **channel_names, float *pixel_buffer);
void oiio_write_pixel_buffer_to_file(const char *filename,u32 width, u32 height,
        u32 channels, float *pixel_buffer, OzyColorSpace colorspace);
#ifdef __cplusplus 
}
#endif

