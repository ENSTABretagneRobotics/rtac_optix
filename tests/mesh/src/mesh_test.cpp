#include <iostream>
using namespace std;

#include <cuda_runtime.h>
#include <optix.h>

#include <rtac_base/type_utils.h>
#include <rtac_base/files.h>
using namespace rtac;
using namespace rtac::files;

#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceMesh.h>
using namespace rtac::cuda;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/AccelerationStruct.h>
#include <rtac_optix/MeshGeometry.h>
using namespace rtac::optix;

#include <mesh_test_rtac_optix/ptx_files.h>

#include "mesh_test.h"

using RaygenRecord     = SbtRecord<RaygenData>;
using MissRecord       = SbtRecord<MissData>;
using ClosestHitRecord = SbtRecord<ClosestHitData>;

int main()
{
    unsigned int W = 800, H = 600;
    auto ptxFiles = mesh_test_rtac_optix::get_ptx_files();

    cudaFree(0); // no-op to initialize cuda
    OPTIX_CHECK(optixInit());

    auto context = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("src/mesh_test.cu", ptxFiles["src/mesh_test.cu"]);
    auto raygenProgram = pipeline->add_raygen_program("__raygen__mesh_test", "src/mesh_test.cu");
    auto missProgram   = pipeline->add_miss_program("__miss__mesh_test", "src/mesh_test.cu");

    auto closestHitProgram = pipeline->add_hit_programs();
    closestHitProgram->set_closesthit({"__closesthit__mesh_test", pipeline->module("src/mesh_test.cu")});
    
    // linking pipeline
    //pipeline->link();

    //// Building mesh acceleration structure
    //auto mesh = DeviceMesh<>::cube();

    //auto buildOptions = rtac::zero<OptixAccelBuildOptions>();
    //buildOptions.buildFlags = OPTIX_BUILD_FLAG_NONE;
    //buildOptions.operation  = OPTIX_BUILD_OPERATION_BUILD;

    //auto buildInput = rtac::zero<OptixBuildInput>();
    //CUdeviceptr vertexData = reinterpret_cast<CUdeviceptr>(mesh.points().data());
    //CUdeviceptr faceData   = reinterpret_cast<CUdeviceptr>(mesh.faces().data());
    //const uint32_t inputFlags[1] = {OPTIX_GEOMETRY_FLAG_NONE};
    //buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    //buildInput.triangleArray.numVertices         = mesh.num_points();
    //buildInput.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
    //buildInput.triangleArray.vertexStrideInBytes = sizeof(DeviceMesh<>::Point);
    //buildInput.triangleArray.vertexBuffers       = &vertexData;
    //buildInput.triangleArray.numIndexTriplets    = mesh.num_faces();
    //buildInput.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    //buildInput.triangleArray.indexStrideInBytes  = sizeof(DeviceMesh<>::Face);
    //buildInput.triangleArray.indexBuffer         = faceData;
    //buildInput.triangleArray.flags               = inputFlags;
    //buildInput.triangleArray.numSbtRecords       = 1;

    //auto handle = AccelerationStruct::Create(context, buildInput, buildOptions);

    auto handle = MeshGeometry::CreateCube(context);
    handle->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE});
    std::vector<float> pose({1.0f,0.0f,0.0f,0.0f,
                             0.0f,1.0f,0.0f,0.0f,
                             0.0f,0.0f,1.0f,2.0f});
    handle->set_pre_transform(pose);

    // Building shader binding table
    auto sbt = rtac::zero<OptixShaderBindingTable>();
    RaygenRecord raygenRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(*raygenProgram, &raygenRecord));
    cudaMalloc((void**)&sbt.raygenRecord, sizeof(RaygenRecord));
    cudaMemcpy((void*)sbt.raygenRecord, &raygenRecord, sizeof(RaygenRecord),
               cudaMemcpyHostToDevice);

    MissRecord missRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(*missProgram, &missRecord));
    cudaMalloc((void**)&sbt.missRecordBase, sizeof(MissRecord));
    cudaMemcpy((void*)sbt.missRecordBase, &missRecord, sizeof(MissRecord),
               cudaMemcpyHostToDevice);
    sbt.missRecordStrideInBytes = sizeof(MissRecord);
    sbt.missRecordCount         = 1;

    ClosestHitRecord chRecord;
    OPTIX_CHECK(optixSbtRecordPackHeader(*closestHitProgram, &chRecord));
    cudaMalloc((void**)&sbt.hitgroupRecordBase, sizeof(ClosestHitRecord));
    cudaMemcpy((void*)sbt.hitgroupRecordBase, &chRecord, sizeof(ClosestHitRecord),
               cudaMemcpyHostToDevice);
    sbt.hitgroupRecordStrideInBytes = sizeof(chRecord);
    sbt.hitgroupRecordCount         = 1;

    
    // output buffer
    DeviceVector<uchar3> imgData(W*H);

    auto params = rtac::zero<Params>();
    params.width     = W;
    params.height    = H;
    params.imageData = imgData.data();
    params.cam       = helpers::PinholeCamera::Create({0.0f,0.0f,0.0f}, {5.0f,4.0f,3.0f});
    params.topObject = *handle;

    CUdeviceptr dparams;
    cudaMalloc((void**)&dparams, sizeof(Params));
    cudaMemcpy((void*)dparams, &params, sizeof(Params), cudaMemcpyHostToDevice);
    
    OPTIX_CHECK(optixLaunch(*pipeline, 0,
                            dparams,
                            sizeof(params), &sbt, W,H,1));
    cudaDeviceSynchronize();

    HostVector<uchar3> output(imgData);

    write_ppm("output.ppm", W, H, reinterpret_cast<const char*>(output.data()));

    return 0;
}


