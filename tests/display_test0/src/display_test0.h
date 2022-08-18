#ifndef _DEF_RTAC_OPTIX_TESTS_DISPLAY_TEST_0_H_
#define _DEF_RTAC_OPTIX_TESTS_DISPLAY_TEST_0_H_

#include <optix.h>

#include <rtac_optix/helpers/PinholeCamera.h>

struct Params
{
    unsigned int width;
    unsigned int height;
    uchar3*      imageData;
    rtac::optix::helpers::PinholeCamera cam;
    OptixTraversableHandle sceneTreeHandle;
};

struct RaygenData
{
    // nothing here for now
};

struct MissData
{
    // nothing here for now
};

struct CubeData
{
    float2* uvCoords;
};

struct SphereData
{
    float radius;
};

struct ClosestHitData
{
    cudaTextureObject_t texObject;
    union {
        CubeData   cube;
        SphereData sphere;
    };
};


#endif //_DEF_RTAC_OPTIX_TESTS_SBT_TEST_0_H_
