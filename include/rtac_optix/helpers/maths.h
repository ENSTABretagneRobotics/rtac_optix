#ifndef _DEF_RTAC_OPTIX_helpers_MATHS_H_
#define _DEF_RTAC_OPTIX_helpers_MATHS_H_

#include <cuda_runtime.h>
#include <optix.h>

#include <rtac_base/cuda/utils.h>

#include <sutil/vec_math.h> // consider replacing this ?

namespace rtac { namespace optix { namespace helpers {


//RTAC_HOSTDEVICE RTAC_INLINE
//float dot(const float3& v0, const float3& v1)
//{
//    return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z;
//}

template <typename T>
RTAC_HOSTDEVICE RTAC_INLINE
int quadratic_solve(T a, T b, T c, T& root1, T& root2)
{
    //root1 = 0;
    //root2 = 0;
    //return 2;
    // solves quadratic equation ax^2 + bx + c = 0
    T delta = b*b - 4.0f*a*c;
    if(delta < 0.0f) {
        return 0;
    }
    else if(delta == 0.0f) {
        root1 = -0.5f*b/a;
        root2 = root1;
        return 1;
    }
    else {
        delta = sqrt(delta);
        root1 = -0.5f*(b + delta) / a;
        root2 = -0.5f*(b - delta) / a;
        return 2;
    }
}

RTAC_HOSTDEVICE RTAC_INLINE
int line_sphere_intersection(const float3& p0, const float3& dir, float radius,
                             float& tmin, float& tmax)
{
    // Find intersection of a sphere centered at the origin and a line.
    // The sphere is parametrized by its radius and the line is parametrized by
    // p0 a point on the line and dir a vector colinear with the line.
    // It comes down to find t such as square_norm(p0 + t*dir) = R^2
    // dir does not have to be normalized but tmin and tmax values will depend
    // on dir length.
    // The return value gives the number of intersections (either 0, 1 if the
    // line is tangent to the sphere (probably will never happen), or 2 if the
    // line cross the sphere).
    return quadratic_solve(dot(dir,dir), 2.0f*dot(dir,p0), dot(p0,p0) - radius*radius,
                           tmin, tmax);
}

#ifdef RTAC_CUDACC

__device__ __forceinline__
float3 get_triangle_hit_position()
{
    // /!\ Will only works if hitting a triangle.
    float3 vertices[3];
    optixGetTriangleVertexData(optixGetGASTraversableHandle(), // current object
                               optixGetPrimitiveIndex(),       // current triangle
                               optixGetSbtGASIndex(),          // ?
                               optixGetRayTmax(),              // Ray time at hit
                               vertices);
    float2 tBary = optixGetTriangleBarycentrics(); // Hit position in the triangle
                                                   // in barycentrics coordinates
    return (1.0f - tBary.x - tBary.y) * vertices[0] +
                              tBary.x * vertices[1] +
                              tBary.y * vertices[2];
}

__device__ __forceinline__
void get_triangle_hit_data(float3& position, float3& normal)
{
    // /!\ Will only works if hitting a triangle.
    float3 vertices[3];
    optixGetTriangleVertexData(optixGetGASTraversableHandle(), // current object
                               optixGetPrimitiveIndex(),       // current triangle
                               optixGetSbtGASIndex(),          // ?
                               optixGetRayTmax(),              // Ray time at hit
                               vertices);
    float2 tBary = optixGetTriangleBarycentrics(); // Hit position in the triangle
                                                   // in barycentrics coordinates
    position = (1.0f - tBary.x - tBary.y) * vertices[0] +
                                  tBary.x * vertices[1] +
                                  tBary.y * vertices[2];
    normal = normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[1]));
}

#endif //RTAC_CUDACC

}; //namespace helpers
}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_helpers_MATHS_H_
