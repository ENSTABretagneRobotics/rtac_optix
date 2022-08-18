#include <rtac_optix/MeshGeometry.h>

namespace rtac { namespace optix {

/**
 * @return a default empty OptixBuildInput with type set to
 *         OPTIX_BUILD_INPUT_TYPE_TRIANGLES.
 */
OptixBuildInput MeshGeometry::default_build_input()
{
    auto res = GeometryAccelStruct::default_build_input();
    res.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    return res;
}

/**
 * Default options (same as AccelerationStruct::default_build_options) :
 * - buildFlags    : OPTIX_BUILD_FLAG_NONE
 * - operation     : OPTIX_BUILD_OPERATION_BUILD
 * - motionOptions : zeroed OptixMotionOptions struct
 * 
 * @return a default OptixAccelBuildOptions for a build operation.
 */
OptixAccelBuildOptions MeshGeometry::default_build_options()
{
    return GeometryAccelStruct::default_build_options();
}

MeshGeometry::MeshGeometry(const Context::ConstPtr& context,
                           const DeviceMesh::ConstPtr& mesh,
                           const DeviceVector<float>& preTransform) :
    GeometryAccelStruct(context, default_build_input(), default_build_options()),
    sourceMesh_(NULL)
{
    this->set_mesh(mesh);
    this->set_pre_transform(preTransform);
}

/**
 * Creates a new instance of MeshGeometry filled with the description of a mesh
 * given by an instance of DeviceMesh.
 *
 * The Constructor of GeometryAccelStruct provides a default material
 * configuration. (All facets have the same material with flag
 * OPTIX_GEOMETRY_FLAG_NONE).
 *
 * @param context      a non-null Context pointer. The Context cannot be
 *                     changed in the object lifetime.
 * @param mesh         DeviceMesh holding the triangular mesh data.
 * @param preTransform global transformation to apply on all vertices. See
 *                     MeshGeometry::set_pre_transform.
 *
 * @return a shared pointer to a new (valid) MeshGeometry instance.
 */
MeshGeometry::Ptr MeshGeometry::Create(const Context::ConstPtr& context,
                                       const DeviceMesh::ConstPtr& mesh,
                                       const DeviceVector<float>& preTransform)
{
    return Ptr(new MeshGeometry(context, mesh, preTransform));
}

/**
 * Creates a new instance of MeshGeometry filled with the description of a cube
 * centered on (0,0,0).
 * 
 * The Constructor of GeometryAccelStruct provides a default material
 * configuration. (All facets have the same material with flag
 * OPTIX_GEOMETRY_FLAG_NONE).
 *
 * @param context      a non-null Context pointer. The Context cannot be
 *                     changed in the object lifetime.
 * @param scale        half-length of the cube edges.
 * @param preTransform global transformation to apply on all vertices. See
 *                     MeshGeometry::set_pre_transform.
 *
 * @return a shared pointer to a new (valid) MeshGeometry instance.
 */
MeshGeometry::Ptr MeshGeometry::CreateCube(const Context::ConstPtr& context,
                                           float scale,
                                           const DeviceVector<float>& preTransform)
{
    return Create(context, DeviceMesh::cube(scale), preTransform);
}

// /**
//  * Creates a new instance of MeshGeometry filled with the description of a mesh
//  * given by an instance of Mesh.
//  *
//  * The Constructor of GeometryAccelStruct provides a default material
//  * configuration. (All facets have the same material with flag
//  * OPTIX_GEOMETRY_FLAG_NONE).
//  *
//  * @param context      a non-null Context pointer. The Context cannot be
//  *                     changed in the object lifetime.
//  * @param mesh         Mesh holding the triangular mesh data.
//  * @param preTransform global transformation to apply on all vertices. See
//  *                     MeshGeometry::set_pre_transform.
//  *
//  * @return a shared pointer to a new (valid) MeshGeometry instance.
//  */
// MeshGeometry::Ptr MeshGeometry::Create(const Context::ConstPtr& context,
//                                        const Mesh& mesh,
//                                        const DeviceVector<float>& preTransform)
// {
//     DeviceMesh::Ptr deviceMesh(new DeviceMesh(mesh));
//     return Ptr(new MeshGeometry(context, deviceMesh, preTransform));
// }

/**
 * Sets the triangular mesh information.
 *
 * The MeshGeometry object keeps ownership of the input mesh via a shared
 * pointer. This is necessary because the OptixBuildInput buildInput_ can only
 * hold raw pointers, so the mesh data must be kept alive at least until the
 * build process ends. After the build process, the shared pointer to the mesh
 * data may safely be released.
 *
 * The mesh information must contain the vertices coordinates, mesh.points().
 * The mesh data can optionally define the faces through the use of index
 * triplets mesh.faces(). (Each index in a triplet refers to a vertex in the
 * mesh, the triplet of vertices describing a single triangle). Alternatively,
 * if no face indexes are given (mesh.faces() == 0), each consecutive triplet
 * of vertices in mesh.points defines a triangle.
 *
 * @param mesh a valid shared pointer to a DeviceMesh instance.
 */
void MeshGeometry::set_mesh(const DeviceMesh::ConstPtr& mesh)
{
    if(mesh->points().size() == 0)
        return;
    
    sourceMesh_ = mesh; // Keeping a reference to mesh to keep it alive.
                        // Can be released after build.

    // Keeping vertex data pointer in a vector is mandatory because the buildInput
    // expect an array of vertex buffers for motion blur calculations.
    if(this->geomData_.size() == 0)
        this->geomData_.resize(1);
    this->geomData_[0] = reinterpret_cast<CUdeviceptr>(mesh->points().data());

    this->buildInput_.triangleArray.numVertices         = mesh->points().size();
    this->buildInput_.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
    this->buildInput_.triangleArray.vertexStrideInBytes = sizeof(DeviceMesh::Point);
    this->buildInput_.triangleArray.vertexBuffers       = geomData_.data();
        

    if(mesh->faces().size() > 0) {
        this->buildInput_.triangleArray.numIndexTriplets    = mesh->faces().size();
        this->buildInput_.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        this->buildInput_.triangleArray.indexStrideInBytes  = sizeof(DeviceMesh::Face);
        this->buildInput_.triangleArray.indexBuffer         =
            reinterpret_cast<CUdeviceptr>(mesh->faces().data());
    }
    else {
        // Erasing possible previously defined index faces.
        this->buildInput_.triangleArray.numIndexTriplets    = 0;
        this->buildInput_.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_NONE;
        this->buildInput_.triangleArray.indexStrideInBytes  = 0;
        this->buildInput_.triangleArray.indexBuffer         = 0;
    }
    this->bump_version();
}

/**
 * Sets a global transformation to apply on all vertices in the build process.
 *
 * The transformation is described by the 3 first rows of an homogeneous matrix
 * (only linear + translation transformations are possible). This
 * transformation will be applied once on the input vertices during the build
 * process. The data held in sourceMesh_ will be unaffected, the operation is
 * performed internally by OptiX.
 *
 * @param preTransform a DeviceVector of size at least 12. Holds the
 *                     coefficients of the first three rows of a row-major
 *                     homogeneous matrix. If the size of preTransform is 0, a
 *                     potentially already set pre_transform is unset.
 */
void MeshGeometry::set_pre_transform(const DeviceVector<float>& preTransform)
{
    if(preTransform.size() == 0) {
        this->unset_pre_transform();
        return;
    }
    if(preTransform.size() != 12) {
        std::ostringstream oss;
        oss << "MeshGeometry : preTransform mush be a 12 sized vector "
            << "(3 first rows of a row major homogeneous matrix).";
        throw std::runtime_error(oss.str());
    }
    preTransform_ = preTransform;
    this->build_input().triangleArray.preTransform = 
        reinterpret_cast<CUdeviceptr>(preTransform_.data());
    this->build_input().triangleArray.transformFormat = OPTIX_TRANSFORM_FORMAT_MATRIX_FLOAT12;
}

void MeshGeometry::unset_pre_transform()
{
    this->build_input().triangleArray.preTransform    = 0;
    this->build_input().triangleArray.transformFormat = OPTIX_TRANSFORM_FORMAT_NONE;
}

/**
 * Allows to access vertices data from an OptiX program (for example to
 * calculate a ray hit location in 3D, which aren't computed by default).
 *
 * This allows the user user to call optixGetTriangleVertexData in a hit
 * program (either \_\_anyhit\_\_ or \_\_closesthit\_\_), this access being
 * deactivated by default. (Probably for performance reason, although we failed
 * to find information on this subject).
 *
 * This adds OPTIX_BUILD_FLAG_ALLOW_RANDOM_VERTEX_ACCESS to the buildFlags in
 * the OptixAccelBuildOptions AccelerationStruct.buildOptions_.
 */
void MeshGeometry::enable_vertex_access()
{
    this->build_options().buildFlags |= OPTIX_BUILD_FLAG_ALLOW_RANDOM_VERTEX_ACCESS;
}

 /**
 * Removes the OPTIX_BUILD_FLAG_ALLOW_RANDOM_VERTEX_ACCESS from the buildFlags
 * in the OptixAccelBuildOptions AccelerationStruct.buildOptions_.
 */
void MeshGeometry::disable_vertex_access()
{
    this->build_options().buildFlags &= ~OPTIX_BUILD_FLAG_ALLOW_RANDOM_VERTEX_ACCESS;
}

/**
 * Returns the total number of primitives in the MeshGeometry. If the triangles
 * are defined with index triplets, the number of triplets is returned.
 * Otherwise, the number of vertices divided by 3 is returned.
 *
 * @return the number of triangles in the MeshGeometry.
 */
unsigned int MeshGeometry::primitive_count() const
{
    // If we have an index buffer set, returning its size. If the index buffer
    // is empty, triangles vertices are assumed to be packed into
    // vertex buffers.
    if(this->buildInput_.triangleArray.numIndexTriplets != 0) {
        return this->buildInput_.triangleArray.numIndexTriplets;
    }
    else {
        return this->buildInput_.triangleArray.numVertices / 3;
    }
}

}; //namespace optix
}; //namespace rtac
