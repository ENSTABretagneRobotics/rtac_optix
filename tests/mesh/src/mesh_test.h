#ifndef _RTAC_OPTIX_TESTS_MESH_TEST_H_
#define _RTAC_OPTIX_TESTS_MESH_TEST_H_

#include <optix.h>

#include <rtac_optix/helpers/PinholeCamera.h>

struct Params
{
    unsigned int width;
    unsigned int height;
    uchar3*      imageData;
    rtac::optix::helpers::PinholeCamera cam;
    OptixTraversableHandle topObject;
};

struct RaygenData
{
    //nothing here for now
};

struct MissData
{
    //nothing here for now
};

struct ClosestHitData
{
    //nothing here for now
};

#endif //_RTAC_OPTIX_TESTS_MESH_TEST_H_
