#include "sbt_indexes0.h"

extern "C" {
    __constant__ Params params;
};

extern "C" __global__ void __raygen__sbt_indexes0()
{
    auto idx = optixGetLaunchIndex();
    auto dim = optixGetLaunchDimensions();

    float3 rayOrigin, rayDirection;
    params.cam.compute_ray(idx, dim, rayOrigin, rayDirection);
    
    RGBRay ray;
    ray.trace(params.topObject, rayOrigin, rayDirection);
    params.output[params.width*idx.y + idx.x] = ray.color;
}

extern "C" __global__ void __miss__sbt_indexes0()
{
    auto data = reinterpret_cast<const MissData*>(optixGetSbtDataPointer());
    RGBRay::set_payload(RGBPayload({make_uchar3(data->value, data->value, data->value)}));
}

extern "C" __global__ void __closesthit__sbt_indexes0()
{
    auto data = reinterpret_cast<const HitData*>(optixGetSbtDataPointer());
    RGBRay::set_payload(RGBPayload({make_uchar3(data->value, data->value, data->value)}));
}
