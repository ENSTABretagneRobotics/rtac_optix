#include <iostream>
using namespace std;

#include <rtac_base/files.h>
using namespace rtac::files;

#include <rtac_base/types/common.h>
#include <rtac_base/types/Mesh.h>
using namespace rtac::types;

#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/Texture2D.h>
using namespace rtac::cuda;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/MeshAccelStruct.h>
using namespace rtac::optix;

#include <rtac_optix_7_sbt_test0/ptx_files.h>

#include "sbt_test0.h"

using RaygenRecord     = SbtRecord<RaygenData>;
using MissRecord       = SbtRecord<MissData>;
using ClosestHitRecord = SbtRecord<ClosestHitData>;

DeviceVector<float2> compute_cube_uv()
{
    auto cube = rtac::types::Mesh<Vector3<float>>::cube(0.5);

    Vector3<float> x0({1.0f,0.0f,0.0f});
    Vector3<float> y0({0.0f,1.0f,0.0f});
    Vector3<float> z0({0.0f,0.0f,1.0f});
    
    std::vector<float2> uv;
    for(auto& f : cube.faces()) {
        auto p0 = cube.point(f.x);
        auto p1 = cube.point(f.y);
        auto p2 = cube.point(f.z);
        
        // a normal vector
        auto n = ((p1 - p0).cross(p2 - p1)).array().abs();
        // Ugly oneliner for index of biggest element out of 3
        int imax = (n[0] > n[1]) ? ((n[0] > n[2]) ? 0 : 2) : (n[1] > n[2]) ? 1 : 2;
        if(imax == 0) {
            uv.push_back(float2({p0[1] + 0.5f, p0[2] + 0.5f}));
            uv.push_back(float2({p1[1] + 0.5f, p1[2] + 0.5f}));
            uv.push_back(float2({p2[1] + 0.5f, p2[2] + 0.5f}));
        }
        else if(imax ==1) {
            uv.push_back(float2({p0[0] + 0.5f, p0[2] + 0.5f}));
            uv.push_back(float2({p1[0] + 0.5f, p1[2] + 0.5f}));
            uv.push_back(float2({p2[0] + 0.5f, p2[2] + 0.5f}));
        }
        else {
            uv.push_back(float2({p0[0] + 0.5f, p0[1] + 0.5f}));
            uv.push_back(float2({p1[0] + 0.5f, p1[1] + 0.5f}));
            uv.push_back(float2({p2[0] + 0.5f, p2[1] + 0.5f}));
        }
    }

    return DeviceVector<float2>(uv);
}

int main()
{
    unsigned int W = 800, H = 600;
    auto ptxFiles = rtac_optix_7_sbt_test0::get_ptx_files();

    rtac::cuda::init_cuda();
    OPTIX_CHECK( optixInit() );

    auto context  = Context::Create();

    // Building pipeline.
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("sbt_test0", ptxFiles["src/sbt_test0.cu"]);
    auto raygenProgram = pipeline->add_raygen_program("__raygen__sbt_test", "sbt_test0");
    auto missProgram   = pipeline->add_miss_program("__miss__sbt_test", "sbt_test0");
    auto hitDesc = rtac::optix::zero<OptixProgramGroupDesc>();
    hitDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitDesc.hitgroup.moduleCH = pipeline->module("sbt_test0");
    hitDesc.hitgroup.entryFunctionNameCH = "__closesthit__sbt_test";
    auto hitProgram = pipeline->add_program_group(hitDesc);
    // At this point the pipeline is not linked and the program are not
    // compiled yet. They will do so when used in an optix API call. (the
    // implicit cast between rtac::optix type and corresponding OptiX native
    // types will trigger compilation / link.
    
    // Simple cube as scene
    auto topObject = MeshAccelStruct::Create(context, MeshAccelStruct::cube_data());
    topObject->add_sbt_flags(OPTIX_GEOMETRY_FLAG_NONE);

    auto checkerboardTex = Texture2D<uchar4>::checkerboard(4,4,
                                                           uchar4({255,255,0,255}),
                                                           uchar4({0,0,255,255}));
    auto uvBuffer = compute_cube_uv();
    
    // setting up sbt
    auto sbt = rtac::optix::zero<OptixShaderBindingTable>();

    RaygenRecord raygenRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*raygenProgram, &raygenRecord) );
    sbt.raygenRecord = reinterpret_cast<CUdeviceptr>(
        rtac::cuda::memcpy::host_to_device(raygenRecord));

    MissRecord missRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*missProgram, &missRecord) );
    sbt.missRecordBase = reinterpret_cast<CUdeviceptr>(
        rtac::cuda::memcpy::host_to_device(missRecord));
    sbt.missRecordCount = 1;
    sbt.missRecordStrideInBytes = sizeof(MissRecord);

    ClosestHitRecord hitRecord;
    hitRecord.data.texObject = checkerboardTex;
    hitRecord.data.uvCoords  = uvBuffer.data();
    OPTIX_CHECK( optixSbtRecordPackHeader(*hitProgram, &hitRecord) );
    sbt.hitgroupRecordBase = reinterpret_cast<CUdeviceptr>(
        rtac::cuda::memcpy::host_to_device(hitRecord));
    sbt.hitgroupRecordCount = 1;
    sbt.hitgroupRecordStrideInBytes = sizeof(ClosestHitRecord);

    DeviceVector<uchar3> imgData(W*H);

    auto params = rtac::optix::zero<Params>();
    params.width     = W;
    params.height    = H;
    params.imageData = imgData.data();
    params.cam       = samples::PinholeCamera::New({0.0f,0.0f,0.0f}, {5.0f,4.0f,3.0f});
    params.sceneTreeHandle = *topObject;

    OPTIX_CHECK( optixLaunch(*pipeline, 0, 
                             reinterpret_cast<CUdeviceptr>(memcpy::host_to_device(params)),
                             sizeof(params), &sbt, W, H, 1) );
    cudaDeviceSynchronize();

    write_ppm("output.ppm", W, H, reinterpret_cast<const char*>(HostVector<uchar3>(imgData).data()));

    return 0;
}
