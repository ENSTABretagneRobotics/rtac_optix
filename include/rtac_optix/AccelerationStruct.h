#ifndef _RTAC_OPTIX_TRAVERSABLE_HANDLE_H_
#define _RTAC_OPTIX_TRAVERSABLE_HANDLE_H_

#include <iostream>
#include <iomanip>
#include <cstring>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/HostVector.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>

namespace rtac { namespace optix {

/**
 * Abstract class representing a node of the object tree which needs to be
 * build by the OptiX API. This class hides the tedious build process from the
 * user.
 *
 * A simple user of the rtac_optix library should not have to directly use
 * this type. More user-friendly types are provided to ease the creation of
 * triangle mesh geometries (MeshGeometry), custom geometries (CustomGeometry),
 * objects (ObjectInstance, or groups of objects (GroupInstance).
 *
 * The nodes of the object tree are either geometrical objects with their own
 * materials (textured triangle meshes, custom geometries such as glass
 * spheres...) or groups of other objects.  Each of the graph nodes needs to be
 * **built** according to the OptiX terminology. In practice, this build
 * operation probably generates the data structures needed by OptiX to perform
 * an efficient ray-tracing operation (generation of KdTrees for fast scene
 * exploration, memory alignment of vertex data for efficient memory
 * access...). From the user perspective, the build process can be viewed as a
 * conversion from user data types to OptiX optimized data types.
 *
 * The build operation of AccelerationStruct is the optixAccelBuild function.
 * See its documentation for more information.
 */
class AccelerationStruct : public OptixWrapper<OptixTraversableHandle>
{
    public:

    using Ptr      = OptixWrapperHandle<AccelerationStruct>;
    using ConstPtr = OptixWrapperHandle<const AccelerationStruct>;

    using BuildInput   = OptixBuildInput;
    using BuildOptions = OptixAccelBuildOptions;
    static BuildInput   default_build_input();
    static BuildOptions default_build_options();

    using Buffer = rtac::cuda::DeviceVector<unsigned char>;
    /** This contains the CUDA stream in which the build is performed (the
     * build operation can take a long time and several build operations can
     * run in parallel in separated CUstreams) and a temporary buffer needed
     * during the build but not used afterwards.  This can be ignored by the
     * user (defaults values will be provided). It is useful if the user wants
     * to optimize the build process.
     */
    struct BuildMeta { Handle<Buffer> buffer; CUstream stream; };

    protected:
    
    Context::ConstPtr  context_;
    mutable BuildInput buildInput_;
    BuildOptions       buildOptions_;
    mutable Buffer     buffer_; // contains data after build
    mutable BuildMeta  buildMeta_;

    virtual void do_build() const;
    void resize_build_buffer(size_t size) const;

    AccelerationStruct(const Context::ConstPtr& context,
                       const BuildInput& buildInput = default_build_input(),
                       const BuildOptions& buildOptions = default_build_options());

    public:

    const BuildInput& build_input() const;
    const BuildOptions& build_options() const;

    BuildInput& build_input();
    BuildOptions& build_options();

    void set_build_buffer(const Handle<Buffer>& buffer);
    void set_build_stream(CUstream stream);
    void set_build_meta(const Handle<Buffer>& buffer, CUstream stream = 0);

    virtual unsigned int sbt_width() const = 0;

    OptixBuildInputType kind() const;
};

}; //namespace optix
}; //namespace rtac

#endif //_RTAC_OPTIX_TRAVERSABLE_HANDLE_H_
