#ifndef _DEF_RTAC_OPTIX_CUSTOM_GEOMETRY_H_
#define _DEF_RTAC_OPTIX_CUSTOM_GEOMETRY_H_

#include <iostream>
#include <iomanip>
#include <cstring>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/cuda/DeviceVector.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/GeometryAccelStruct.h>

namespace rtac { namespace optix {

/**
 * Specialization of GeometryAccelStruct to describe a custom geometry.
 *
 * This can be used as a geometry in an ObjectInstance. A single CustomGeometry
 * can be use in several ObjectInstance.
 *
 * A CustomGeometry is used to describe any geometry which is not a triangular
 * mesh or a set of curves (Curves are not supported by rtac_optix yet). It
 * is a very generic description of an object since it only describes its
 * bounding box (see CustomGeometry::set_aabb). To make a ray intersect with a
 * CustomGeometry, the OptiX library first makes the ray intersect with its
 * bounding box. When a ray enters the bounding box, the OptiX library calls a
 * user defined \_\_intersection\_\_ program which is responsible for reporting
 * to the OptiX API if an intersection happened. Then, the \_\_anyhit\_\_ and
 * \_\_closesthit\_\_ programs are called if intersections were reported.
 *
 * For now, CustomGeometry can only describe a single primitive.
 */
class CustomGeometry : public GeometryAccelStruct
{
    public:

    using Ptr      = OptixWrapperHandle<CustomGeometry>;
    using ConstPtr = OptixWrapperHandle<const CustomGeometry>;
    template <typename T>
    using DeviceVector = rtac::cuda::DeviceVector<T>;

    static OptixBuildInput        default_build_input();
    static OptixAccelBuildOptions default_build_options();

    protected:
    
    // Axis Aligned Bounding Box.
    DeviceVector<float> aabb_;

    CustomGeometry(const Context::ConstPtr& context);

    public:

    static Ptr Create(const Context::ConstPtr& context);

    void set_aabb(const std::vector<float>& aabb);

    virtual unsigned int primitive_count() const;
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_CUSTOM_GEOMETRY_H_
