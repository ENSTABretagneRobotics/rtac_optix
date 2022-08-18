#ifndef _DEF_RTAC_OPTIX_MESH_GEOMETRY_H_
#define _DEF_RTAC_OPTIX_MESH_GEOMETRY_H_

#include <iostream>
#include <iomanip>
#include <cstring>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/DeviceMesh.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/GeometryAccelStruct.h>

namespace rtac { namespace optix {

/**
 * Specialization of GeometryAccelStruct to describe a triangle mesh geometry.
 *
 * This can be used as a geometry in an ObjectInstance. A single MeshGeometry
 * can be use in several ObjectInstance.
 *
 * This object can be used to describe a triangle mesh geometry compatible with
 * the RTX cores on recent NVIDIA GPUs. (The RTX cores specifically accelerates
 * the computation of ray-triangle intersections). If the GPU described in the
 * current Context has no RTX cores the OptiX library will use a software
 * implemented intersection algorithm on regular CUDA cores. This is handled
 * directly into the OptiX library and no user action is required to
 * activate/deactivate the use of RTX cores.
 */
class MeshGeometry : public GeometryAccelStruct
{
    public:

    using Ptr        = OptixWrapperHandle<MeshGeometry>;
    using ConstPtr   = OptixWrapperHandle<const MeshGeometry>;
    using DeviceMesh = rtac::cuda::DeviceMesh<>;
    template <typename T>
    using DeviceVector = rtac::cuda::DeviceVector<T>;

    static OptixBuildInput        default_build_input();
    static OptixAccelBuildOptions default_build_options();

    static DeviceMesh::Ptr cube_data(float scale = 1.0) { return DeviceMesh::cube(scale); }

    protected:

    DeviceMesh::ConstPtr sourceMesh_;
    DeviceVector<float>    preTransform_;  // Row-major homogeneous matrix without bottom line.

    MeshGeometry(const Context::ConstPtr& context,
                 const DeviceMesh::ConstPtr& mesh,
                 const DeviceVector<float>& preTransform = DeviceVector<float>(0));

    public:

    static Ptr Create(const Context::ConstPtr& context,
                      const DeviceMesh::ConstPtr& mesh,
                      const DeviceVector<float>& preTransform = DeviceVector<float>(0));
    static Ptr CreateCube(const Context::ConstPtr& context,
                          float scale = 1.0f,
                          const DeviceVector<float>& preTransform = DeviceVector<float>(0));
    //static Ptr Create(const Context::ConstPtr& context,
    //                  const Mesh& mesh,
    //                  const DeviceVector<float>& preTransform = DeviceVector<float>(0));

    void set_mesh(const DeviceMesh::ConstPtr& mesh);

    void set_pre_transform(const DeviceVector<float>& preTransform);
    void unset_pre_transform();

    void enable_vertex_access();
    void disable_vertex_access();

    virtual unsigned int primitive_count() const;

    unsigned int num_points() const { return sourceMesh_->points().size(); }
    unsigned int num_faces()  const { return sourceMesh_->faces().size();  }
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_MESH_GEOMETRY_H_
