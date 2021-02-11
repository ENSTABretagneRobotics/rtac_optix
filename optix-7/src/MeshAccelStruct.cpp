#include <rtac_optix/MeshAccelStruct.h>

namespace rtac { namespace optix {

OptixBuildInput MeshAccelStruct::default_build_input()
{
    auto res = AccelerationStruct::default_build_input();
    res.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    return res;
}

OptixAccelBuildOptions MeshAccelStruct::default_build_options()
{
    return AccelerationStruct::default_build_options();
}

Handle<MeshAccelStruct::DeviceMesh> MeshAccelStruct::cube_data(float scale)
{
    return Handle<DeviceMesh>(new DeviceMesh(rtac::types::Mesh<float3,uint3>::cube(scale)));
}

MeshAccelStruct::MeshAccelStruct(const Context::ConstPtr& context,
                                 const Handle<const DeviceMesh>& mesh,
                                 const DeviceVector<float>& preTransform,
                                 const std::vector<unsigned int>& sbtFlags) :
    AccelerationStruct(context, default_build_input(), default_build_options()),
    sourceMesh_(NULL),
    vertexBuffers_(1)
{
    this->set_mesh(mesh);
    this->set_pre_transform(preTransform);
    this->set_sbt_flags(sbtFlags);
}

MeshAccelStruct::Ptr MeshAccelStruct::Create(const Context::ConstPtr& context,
                                             const Handle<const DeviceMesh>& mesh,
                                             const DeviceVector<float>& preTransform,
                                             const std::vector<unsigned int>& sbtFlags)
{
    return Ptr(new MeshAccelStruct(context, mesh, preTransform, sbtFlags));
}

void MeshAccelStruct::set_mesh(const Handle<const DeviceMesh>& mesh)
{
    if(mesh->num_points() == 0)
        return;

    sourceMesh_ = mesh; // Keeping a reference to mesh to keep it alive.
                        // Can be released after build.

    // Keeping vertex data pointer in a vector is mandatory because the buildInput
    // expect an array of vertex buffers for motion blur calculations.
    vertexBuffers_[0] = reinterpret_cast<CUdeviceptr>(mesh->points().data());

    this->buildInput_.triangleArray.numVertices         = mesh->num_points();
    this->buildInput_.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
    this->buildInput_.triangleArray.vertexStrideInBytes = sizeof(DeviceMesh::Point);
    this->buildInput_.triangleArray.vertexBuffers       = vertexBuffers_.data();
        

    if(mesh->num_faces() > 0) {
        this->buildInput_.triangleArray.numIndexTriplets    = mesh->num_faces();
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
}

void MeshAccelStruct::set_pre_transform(const DeviceVector<float>& preTransform)
{
    if(preTransform.size() == 0) {
        this->unset_pre_transform();
        return;
    }
    if(preTransform.size() != 12) {
        std::ostringstream oss;
        oss << "MeshAccelStruct : preTransform mush be a 12 sized vector "
            << "(3 first rows of a row major homogeneous matrix).";
        throw std::runtime_error(oss.str());
    }
    preTransform_ = preTransform;
    this->buildInput_.triangleArray.preTransform = 
        reinterpret_cast<CUdeviceptr>(preTransform_.data());
    this->buildInput_.triangleArray.transformFormat = OPTIX_TRANSFORM_FORMAT_MATRIX_FLOAT12;
}

void MeshAccelStruct::set_sbt_flags(const std::vector<unsigned int>& flags)
{
    sbtFlags_ = flags;
    this->buildInput_.triangleArray.flags = sbtFlags_.data();
    this->buildInput_.triangleArray.numSbtRecords = sbtFlags_.size();
}

void MeshAccelStruct::add_sbt_flags(unsigned int flag)
{
    sbtFlags_.push_back(flag);
    this->buildInput_.triangleArray.flags = sbtFlags_.data();
    this->buildInput_.triangleArray.numSbtRecords = sbtFlags_.size();
}

void MeshAccelStruct::unset_pre_transform()
{
    this->buildInput_.triangleArray.preTransform    = 0;
    this->buildInput_.triangleArray.transformFormat = OPTIX_TRANSFORM_FORMAT_NONE;
}

void MeshAccelStruct::unset_sbt_flags()
{
    sbtFlags_.clear();
    this->buildInput_.triangleArray.flags = nullptr;
    this->buildInput_.triangleArray.numSbtRecords = 0;
}

}; //namespace optix
}; //namespace rtac
