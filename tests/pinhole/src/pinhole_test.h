#ifndef _DEF_RTAC_OPTIX_TESTS_PINHOLE_TEST_H_
#define _DEF_RTAC_OPTIX_TESTS_PINHOLE_TEST_H_

#include <optix.h>

#include <rtac_optix/helpers/PinholeCamera.h>

struct Params
{
    // image data
    unsigned int width;
    unsigned int height;
    float*       imageData;

    // camera
    rtac::optix::helpers::PinholeCamera cam;
};

struct RaygenData
{
    // no parameters for now
};

struct MissData
{
    // no parameters for now
};

#endif //_DEF_RTAC_OPTIX_TESTS_PINHOLE_TEST_H_
