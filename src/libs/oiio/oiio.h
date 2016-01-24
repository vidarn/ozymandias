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
u8 oiio_get_image_info(const char *filename, u32 *w, u32 *h, u32 *channels);
void oiio_read_image(const char *filename, float *pixels);
#ifdef __cplusplus 
}
#endif

