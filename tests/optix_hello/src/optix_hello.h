#ifndef _DEF_OPTIX_HELLO_H_
#define _DEF_OPTIX_HELLO_H_

struct Params
{
    uchar4* image;
    unsigned int image_width;
};

struct RayGenData
{
    float r,g,b;
};

#endif //_DEF_OPTIX_HELLO_H_
