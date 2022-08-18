#include <optix.h>

#include "pinhole_test.h"

extern "C" {
    __constant__ Params params;
}

extern "C" __global__ void __miss__pinhole()
{
}

extern "C" __global__ void __raygen__pinhole()
{
    const uint3 idx = optixGetLaunchIndex();
    const uint3 dim = optixGetLaunchDimensions();
    
    float3 rayOrigin, rayDirection;
    params.cam.compute_ray(idx, dim, rayOrigin, rayDirection);

    //params.imageData[idx.y * params.width + idx.x] = rayDirection.x;
    params.imageData[idx.y * params.width + idx.x] = rayDirection.y;
    //params.imageData[idx.y * params.width + idx.x] = rayDirection.z;

    //params.imageData[idx.y * params.width + idx.x] = ((float)idx.x) / (params.width  - 1);
    //params.imageData[idx.y * params.width + idx.x] = ((float)idx.y) / (params.height - 1);
}
