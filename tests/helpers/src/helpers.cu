#include "helpers.h"

extern "C" {
    __constant__ Params params;
};

extern "C" __global__ void __raygen__helpers()
{
    auto idx  = optixGetLaunchIndex();
    auto dims = optixGetLaunchDimensions();

    float3 origin, dir;
    params.cam.compute_ray(idx, dims, origin, dir);

    RGBRay ray;
    ray.trace(params.topObject, origin, dir);
    params.output[idx] = ray;
    //params.output.begin()[dims.x*idx.y + idx.x] = ray;
}

extern "C" __global__ void __miss__helpers_rgb()
{
    //auto color = (const RGBHitData*)optixGetSbtDataPointer();
    //RGBRay::set_payload(color->color);
    RGBRay::set_payload({1,0,1});
}

extern "C" __global__ void __miss__helpers_shadow()
{
    ShadowRay::set_payload({false});
}

extern "C" __global__ void __closesthit__helpers_rgb()
{
    auto color = (const RGBHitData*)optixGetSbtDataPointer();
    RGBRay::set_payload(color->color);
}



