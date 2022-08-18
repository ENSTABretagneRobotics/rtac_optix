#ifndef _DEF_RTAC_OPTIX_TESTS_HELPERS_H_
#define _DEF_RTAC_OPTIX_TESTS_HELPERS_H_

#include <optix.h>

#include <rtac_optix/RaytypeFactory.h>
#include <rtac_optix/Material.h>

#include <rtac_optix/helpers/RenderBuffer.h>
#include <rtac_optix/helpers/PinholeCamera.h>
using namespace rtac::optix;
using namespace rtac::optix::helpers;

struct Params {
    RenderBuffer<float3>   output;
    PinholeCamera          cam;
    OptixTraversableHandle topObject;
};

// RayType definition
using RGBPayload = float3;
struct ShadowPayload {
    bool hit;
};

using Raytypes  = RaytypeFactory<RGBPayload, ShadowPayload>;
using RGBRay    = Raytypes::Raytype<0>;
using ShadowRay = Raytypes::Raytype<1>;

// Material definition
struct RGBHitData {
    float3 color;
};

using RGBMaterial    = Material<RGBRay, RGBHitData>;
using ShadowMaterial = Material<ShadowRay, void>;

#endif //_DEF_RTAC_OPTIX_TESTS_HELPERS_H_
