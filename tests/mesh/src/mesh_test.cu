#include <optix.h>

#include "mesh_test.h"


extern "C" {
    __constant__ Params params;
}

extern "C" __global__ void __raygen__mesh_test()
{
    const uint3 idx = optixGetLaunchIndex();
    const uint3 dim = optixGetLaunchDimensions();

    float3 rayOrigin, rayDirection;
    params.cam.compute_ray(idx, dim, rayOrigin, rayDirection);
    
    uint32_t p0, p1, p2;
    optixTrace(params.topObject, 
               rayOrigin, rayDirection,
               0.0f, 1e8f, // tmin,tmax = ray time bounds
               0.0f, // ray time (used for motion blur)
               OptixVisibilityMask(255), // always visible
               OPTIX_RAY_FLAG_NONE,
               0,1,0, // SBT offset, stride, missSBToffset
               p0, p1, p2);
    params.imageData[params.width*idx.y + idx.x] = make_uchar3(p0,p1,p2);
}

extern "C" __global__ void __miss__mesh_test()
{
    optixSetPayload_0(255);
    optixSetPayload_1(255);
    optixSetPayload_2(0);
}

extern "C" __global__ void __closesthit__mesh_test()
{
    optixSetPayload_0(0);
    optixSetPayload_1(0);
    optixSetPayload_2(255);
}



