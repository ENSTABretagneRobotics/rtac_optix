#ifndef _DEF_RTAC_OPTIX_TESTS_SBT_INDEXES_TESTS_0_H_
#define _DEF_RTAC_OPTIX_TESTS_SBT_INDEXES_TESTS_0_H_

#include <optix.h>

#include <rtac_optix/Raytype.h>
#include <rtac_optix/RaytypeFactory.h>

#include <rtac_optix/helpers/PinholeCamera.h>

struct Params {
    unsigned int   width;
    unsigned int   height;
    //unsigned char* output;
    uchar3*        output;
    rtac::optix::helpers::PinholeCamera cam;
    OptixTraversableHandle topObject;
};

struct RaygenData {};
struct MissData {
    unsigned int value;
};

struct HitData {
    unsigned int value;
};

// RayTypes tests
struct RGBPayload {
    uchar3 color;
};
struct ShadowPayload {
    bool hit;
};
using Raytypes = rtac::optix::RaytypeFactory<RGBPayload, ShadowPayload>;

using RGBRay    = Raytypes::Raytype<0>;
using ShadowRay = Raytypes::Raytype<1>;



#endif //_DEF_RTAC_OPTIX_TESTS_SBT_INDEXES_TESTS_0_H_
