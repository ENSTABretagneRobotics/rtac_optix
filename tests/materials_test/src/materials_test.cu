#include "materials_test.h"

extern "C" {
    __constant__ Params params;
};

extern "C" __global__ void __raygen__materials_test()
{
    uint3 idx  = optixGetLaunchIndex();
    uint3 dims = optixGetLaunchDimensions();

    float3 rayOrigin, rayDirection;
    params.cam.compute_ray(idx, dims, rayOrigin, rayDirection);
    RGBRay ray;
    ray.trace(params.topObject, rayOrigin, rayDirection);
    params.imgData[params.width * idx.y + idx.x] = ray;
}

extern "C" __global__ void __miss__materials_test()
{
    auto data = reinterpret_cast<const HitData*>(optixGetSbtDataPointer());
    RGBRay::set_payload(data->color);
}

extern "C" __global__ void __closesthit__materials_test()
{
    auto data = reinterpret_cast<const HitData*>(optixGetSbtDataPointer());
    RGBRay::set_payload(data->color);
}



