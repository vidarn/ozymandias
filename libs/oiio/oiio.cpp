#include "oiio.h"
#include "../../math_common.h"
#include <OpenImageIO/imageio.h>
#include <stdio.h>
OIIO_NAMESPACE_USING

u8 oiio_format_supports_passes(const char *filename)
{
    ImageOutput *out = ImageOutput::create (filename);
    return out && out->supports("channelformats");
}

void oiio_write_passes_file(const char *filename, u32 width, u32 height,
        u32 channels, const char **channel_names, float *pixel_buffer)
{
    ImageOutput *out = ImageOutput::create(filename);
    ImageSpec spec((int)width, (int)height, (int)channels, TypeDesc::FLOAT);
    for(u32 c=0; c<channels; c++){
        spec.channelformats.push_back (TypeDesc::FLOAT);
    }
    spec.channelnames.clear ();
    for(u32 c=0; c<channels; c++){
        spec.channelnames.push_back (channel_names[c]);
    }
    out->open (filename, spec);
    out->write_image (TypeDesc::UNKNOWN, pixel_buffer, channels*sizeof(float));
}

void oiio_write_pixel_buffer_to_file(const char *filename, u32 width, u32 height, 
        u32 channels, float *pixel_buffer, OzyColorSpace colorspace)
{
    ImageOutput *out = ImageOutput::create (filename);
    if (out){
        ImageSpec spec ((int)width, (int)height, (int)channels);
        switch(colorspace){
            case OZY_COLORSPACE_LINEAR:
                spec.attribute("oiio:ColorSpace","Linear");
                break;
            case OZY_COLORSPACE_SRGB:
                spec.attribute("oiio:ColorSpace","sRGB");
                break;
        }
        out->open (filename, spec);
        out->write_image (TypeDesc::FLOAT, pixel_buffer);
        out->close ();
    } else {
        printf("Error: Writing file \"%s\" failed!\n",filename);
    }
}
