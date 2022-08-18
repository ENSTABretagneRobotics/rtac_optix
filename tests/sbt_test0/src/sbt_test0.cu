#include "sbt_test0.h"

#include <rtac_optix/helpers/maths.h>

using namespace rtac::optix;

extern "C" {
    __constant__ Params params;
}

extern "C" __global__ void __raygen__sbt_test()
{
    const uint3 idx = optixGetLaunchIndex();
    const uint3 dim = optixGetLaunchDimensions();

    float3 rayOrigin, rayDirection;
    params.cam.compute_ray(idx, dim, rayOrigin, rayDirection);

    uint32_t p0, p1, p2;
    optixTrace(params.sceneTreeHandle,
               rayOrigin, rayDirection,
               0.0f, 1e8f, // tmin,tmax
               0.0f, // raytime
               OptixVisibilityMask(255),
               OPTIX_RAY_FLAG_NONE,
               0,1,0, // SBT offset, stride, missSBToffset
               p0, p1, p2);
    params.imageData[params.width*idx.y + idx.x] = make_uchar3(p0,p1,p2);
}

extern "C" __global__ void __miss__sbt_test()
{
    optixSetPayload_0(25);
    optixSetPayload_1(25);
    optixSetPayload_2(25);
}

extern "C" __global__ void __closesthit__sbt_test()
{
    auto recordData = reinterpret_cast<ClosestHitData*>(optixGetSbtDataPointer());
    
    // There is 3 uv coordinates for each triangle (on per vertex) so
    // memory offset is 3*primitiveIndex;
    float2* vertexUVs = recordData->cube.uvCoords + 3*optixGetPrimitiveIndex();

    float2 b = optixGetTriangleBarycentrics();
    float2 uv = (1.0f - b.x - b.y) * vertexUVs[0]
              +                b.x * vertexUVs[1]
              +                b.y * vertexUVs[2];

    float4 color = tex2D<float4>(recordData->texObject, uv.x, uv.y);

    optixSetPayload_0((uint32_t)255*color.x);
    optixSetPayload_1((uint32_t)255*color.y);
    optixSetPayload_2((uint32_t)255*color.z);
}

extern "C" __global__ void __intersection__sphere()
{
    auto sphere = reinterpret_cast<const ClosestHitData*>(optixGetSbtDataPointer())->sphere;
    
    // Intersection will be calculated in object reference frame (sphere center
    // at the origin, so only the radius is needed to calculate the intersection).
    auto rayOrigin    = optixGetObjectRayOrigin();
    auto rayDirection = optixGetObjectRayDirection();

    float tmin, tmax;
    if(helpers::line_sphere_intersection(rayOrigin, rayDirection,
                                         sphere.radius,
                                         tmin, tmax) > 0) {
        float rayTmin = optixGetRayTmin();
        float rayTmax = optixGetRayTmax();
        if(rayTmin < tmin && tmin < rayTmax) {
            // Computing normal vector (since we are in object space, the
            // normal vector is colinear to the intersection point position).
            float3 n = normalize(rayOrigin + tmin * rayDirection);
            optixReportIntersection(tmin,
                                    OPTIX_PRIMITIVE_TYPE_CUSTOM,
                                    __float_as_int(n.x),
                                    __float_as_int(n.y),
                                    __float_as_int(n.z));
        }
        else if(rayTmin < tmax && tmax < rayTmax) {
            // Computing normal vector (since we are in object space, the
            // normal vector is colinear to the intersection point position).
            float3 n = normalize(rayOrigin + tmax * rayDirection);
            optixReportIntersection(tmax,
                                    OPTIX_PRIMITIVE_TYPE_CUSTOM,
                                    __float_as_int(n.x),
                                    __float_as_int(n.y),
                                    __float_as_int(n.z));
        }
    }
}

extern "C" __global__ void __closesthit__sphere()
{
    auto recordData = reinterpret_cast<const ClosestHitData*>(optixGetSbtDataPointer());
    
    float3 n = make_float3(__int_as_float(optixGetAttribute_0()),
                           __int_as_float(optixGetAttribute_1()),
                           __int_as_float(optixGetAttribute_2()));
    
    float4 color = tex2D<float4>(recordData->texObject,
                                 0.5f*atan2f(n.y, n.x) / M_PIf,
                                 0.5f*(n.z + 1.0f));

    optixSetPayload_0((uint32_t)255*color.x);
    optixSetPayload_1((uint32_t)255*color.y);
    optixSetPayload_2((uint32_t)255*color.z);
}



