//TODO(Vidar): wrap OIIO
/*OIIO_NAMESPACE_USING

void write_pixel_buffer_to_file(const char *filename,int width, int height, 
        float *pixel_buffer, int num_subsamples)
{
    float inv_num_subsamples = 1.f/(float)num_subsamples;
    const int channels = 3;
    int pixel_buffer_size = width*height*channels;
    unsigned char *pixels = malloc(pixel_buffer_size);
    for(int j=0;j<pixel_buffer_size;j++){
        pixels[j] = to_pixel_value(pixel_buffer[j]*inv_num_subsamples);
        //pixels[j] = 255;
    }

    ImageOutput *out = ImageOutput::create (filename);
    if (out){
        ImageSpec spec (width, height, channels, TypeDesc::UINT8);
        out->open (filename, spec);
        out->write_image (TypeDesc::UINT8, pixels);
        out->close ();
        //ImageOutput::destroy (out);
    }
    free(pixels);
}*/
