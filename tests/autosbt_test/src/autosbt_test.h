#ifndef _DEF_RTAC_OPTIX_TEST_AUTOSBT_TEST_H_
#define _DEF_RTAC_OPTIX_TEST_AUTOSBT_TEST_H_

#include <rtac_optix/RaytypeFactory.h>
#include <rtac_optix/Material.h>
using namespace rtac::optix;

#include <rtac_optix/helpers/maths.h>
#include <rtac_optix/helpers/PinholeCamera.h>
using PinholeCamera = rtac::optix::helpers::PinholeCamera;

struct Params {
    unsigned int width;
    unsigned int height;
    uchar3* output;
    PinholeCamera cam;
    OptixTraversableHandle topObject;
};

using RgbPayload = uchar3;
struct RgbMissData {
    RgbPayload color;
};
struct RgbHitData {
    RgbPayload color;
    float3     light;
};

struct ShadowPayload {
    float tHit;
};

using Raytypes  = RaytypeFactory<RgbPayload, ShadowPayload>;
using RgbRay    = Raytypes::Raytype<0>;
using ShadowRay = Raytypes::Raytype<1>;

using RgbMaterial     = Material<RgbRay,RgbHitData>;
using RgbMissMaterial = Material<RgbRay,RgbMissData>;
using ShadowMaterial = Material<ShadowRay,void>;

#endif //_DEF_RTAC_OPTIX_TEST_AUTOSBT_TEST_H_
