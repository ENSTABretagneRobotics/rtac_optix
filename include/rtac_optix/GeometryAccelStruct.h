#ifndef _DEF_RTAC_OPTIX_GEOMETRY_ACCEL_STRUCT_H_
#define _DEF_RTAC_OPTIX_GEOMETRY_ACCEL_STRUCT_H_

#include <iostream>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/cuda/DeviceVector.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/AccelerationStruct.h>

namespace rtac { namespace optix {

/**
 * Abstract specialization of AccelerationStruct describing the geometry of a
 * physical object the rays will intersect with.
 *
 * GeometryAccelStruct is the base class for MeshGeometry and CustomGeometry
 * which respectively describes a triangle mesh object (optimized for
 * triangle-ray intersection using the NVIDIA RTX cores), and a custom geometry
 * object (usually one or a set of parametrized surfaces, such as spheres,
 * cones, quadrics ...). A GeometryAccelStruct also defines the material
 * structure of the geometry. A material index (together with material specific
 * flags) must be defined for each primitive (Or a single one for all
 * primitives).
 *
 * It is important to understand that the GeometryAccelStruct only associate
 * each primitive with a **material index or label**, not with a Material
 * object instance. The user himself must provide the association between a
 * **material index** and a Material instance.
 * 
 * Example : a GeometryAccelStruct (rather the MeshGeometry subclass)
 * represents the triangle mesh of a house. The Material indexes that the
 * GeometryAccelStruct holds are :
 * - 0 : a rocky wall
 * - 1 : a tiled roof
 * - 2 : a glass window.
 *
 * To each triangle is associated an index (0, 1 or 2) which says if a triangle
 * is a rocky wall, a tiled roof or a glass window. However, it does not says
 * **how** the ray is supposed to interact with a rock, a tile, or glass. The
 * **how** is defined in a \_\_anythit\_\_ or a \_\_closesthit\_\_ program
 * embedded within a Material instance.  The association between a
 * GeometryAccelStruct and the Material instances is done in the ObjectInstance
 * class.
 */
class GeometryAccelStruct : public AccelerationStruct
{
    public:

    using Ptr                 = OptixWrapperHandle<GeometryAccelStruct>;
    using ConstPtr            = OptixWrapperHandle<const GeometryAccelStruct>;
    using Buffer              = AccelerationStruct::Buffer;
    using MaterialIndexBuffer = rtac::cuda::DeviceVector<uint8_t>;

    using BuildInput   = AccelerationStruct::BuildInput;
    using BuildOptions = AccelerationStruct::BuildOptions;
    static BuildInput   default_build_input();
    static BuildOptions default_build_options();
    static std::vector<unsigned int> default_hit_flags();
    
    private:

    void update_hit_setup() const;
    virtual void do_build() const;

    protected:

    std::vector<CUdeviceptr>    geomData_;
    std::vector<unsigned int>   materialHitFlags_;
    Handle<MaterialIndexBuffer> materialIndexes_;


    GeometryAccelStruct(const Context::ConstPtr& context,
                        const OptixBuildInput& buildInput = default_build_input(),
                        const OptixAccelBuildOptions& options = default_build_options());

    public:
    
    void material_hit_setup(const std::vector<unsigned int>& hitFlags,
                            const Handle<MaterialIndexBuffer>& materialIndexes = nullptr);
    void material_hit_setup(const std::vector<unsigned int>& hitFlags,
                            const std::vector<uint8_t>& materialIndexes);
    void clear_hit_setup();

    virtual unsigned int sbt_width() const;

    virtual unsigned int primitive_count() const = 0;
};

}; //namespace optix
}; //namespace rtac


#endif //_DEF_RTAC_OPTIX_GEOMETRY_ACCEL_STRUCT_H_
