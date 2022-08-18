#include <optix.h>

#include "optix_hello.h"
//#include <cuda/helpers.h>

extern "C" {
__constant__ Params params;
}

extern "C"
__global__ void __raygen__draw_solid_color()
{
    uint3 launch_index = optixGetLaunchIndex();
    RayGenData* rtData = (RayGenData*)optixGetSbtDataPointer();

    params.image[launch_index.y * params.image_width + launch_index.x] =
        make_uchar4((unsigned char)(255 * rtData->r),
                    (unsigned char)(255 * rtData->g),
                    (unsigned char)(255 * rtData->b),
                    255u);

        //make_color( make_float3( rtData->r, rtData->g, rtData->b ) );
}

