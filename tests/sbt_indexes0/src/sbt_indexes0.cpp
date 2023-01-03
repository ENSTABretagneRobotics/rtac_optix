#include <iostream>
using namespace std;

#include <rtac_base/type_utils.h>
#include <rtac_base/files.h>
using namespace rtac;

#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
using namespace rtac::cuda;

#include <rtac_optix/utils.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/MeshGeometry.h>
#include <rtac_optix/InstanceAccelStruct.h>
using namespace rtac::optix;

#include <sbt_indexes0_rtac_optix/ptx_files.h>

#include "sbt_indexes0.h"

using RaygenRecord = SbtRecord<RaygenData>;
using MissRecord   = SbtRecord<MissData>;
using HitRecord    = SbtRecord<HitData>;

template<typename T>
void print_output(unsigned int W, unsigned int H, const DeviceVector<T>& output);

int main()
{
    cout << "RayType count    : " << Raytypes::RaytypeCount << endl;
    cout << "RGBRay index     : " << RGBRay::Index          << endl;
    cout << "ShadowRay index  : " << ShadowRay::Index       << endl;

    optix_init();
    auto context  = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("sbt_indexes0",
                         sbt_indexes0_rtac_optix::get_ptx_files()["src/sbt_indexes0.cu"]);
    auto raygen = pipeline->add_raygen_program("__raygen__sbt_indexes0", "sbt_indexes0");
    auto miss   = pipeline->add_miss_program("__miss__sbt_indexes0", "sbt_indexes0");

    auto hitGroup = pipeline->add_hit_programs();
    hitGroup->set_closesthit({"__closesthit__sbt_indexes0", pipeline->module("sbt_indexes0")});

    auto cubeMesh = MeshGeometry::cube_data();
    auto cube0    = MeshGeometry::Create(context, cubeMesh);
    cube0->material_hit_setup({{OPTIX_GEOMETRY_FLAG_NONE}});

    // building a per-triangle material (=array of sbt index offsets)
    std::shared_ptr<DeviceVector<unsigned char>> sbtIndexOffsets(new DeviceVector<unsigned char>(
        std::vector<unsigned char>({0,2,0,2,0,2,0,2,0,2,0,2})));
    cube0->material_hit_setup(std::vector<unsigned int>(3, OPTIX_GEOMETRY_FLAG_NONE),
                              sbtIndexOffsets);

    auto inst0 = Instance::Create(cube0);
    auto inst1 = Instance::Create(cube0);
    //inst1->set_sbt_offset(200);
    inst1->set_transform({1,0,0,-4,
                          0,1,0, 2,
                          0,0,1, 0});
    auto topObject = InstanceAccelStruct::Create(context);
    topObject->add_instance(inst0);
    topObject->add_instance(inst1);

    auto inst2 = Instance::Create(topObject);
    inst2->set_transform({1,0,0,0,
                          0,1,0,0,
                          0,0,1,3});
    auto topTopObject = InstanceAccelStruct::Create(context);
    topTopObject->add_instance(inst2);

    auto sbt = rtac::zero<OptixShaderBindingTable>();

    RaygenRecord raygenRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*raygen, &raygenRecord) );
    cudaMalloc((void**)&sbt.raygenRecord, sizeof(RaygenRecord));
    cudaMemcpy((void*)sbt.raygenRecord, &raygenRecord, sizeof(RaygenRecord),
               cudaMemcpyHostToDevice);

    std::vector<MissRecord> missRecords(255);
    for(int i = 0; i < missRecords.size(); i++) {
        missRecords[i].data.value = i;
        OPTIX_CHECK( optixSbtRecordPackHeader(*miss, &missRecords[i]) );
    }
    DeviceVector<MissRecord> dMissRecords(missRecords);
    sbt.missRecordBase          = (CUdeviceptr)dMissRecords.data();
    sbt.missRecordCount         = dMissRecords.size();
    sbt.missRecordStrideInBytes = sizeof(MissRecord);
    
    //std::vector<HitRecord> hitRecords(255);
    std::vector<HitRecord> hitRecords(127);
    for(int i = 0; i < hitRecords.size(); i++) {
        hitRecords[i].data.value = hitRecords.size() - 1 - i;
        OPTIX_CHECK( optixSbtRecordPackHeader(*hitGroup, &hitRecords[i]) );
    }
    DeviceVector<HitRecord> dHitRecords(hitRecords);
    sbt.hitgroupRecordBase          = (CUdeviceptr)dHitRecords.data();
    sbt.hitgroupRecordCount         = hitRecords.size();
    sbt.hitgroupRecordStrideInBytes = sizeof(HitRecord);

    //unsigned int W = 32, H = 24;
    //DeviceVector<unsigned char> output(W*H);

    unsigned int W = 800, H = 600;
    DeviceVector<uchar3> output(W*H);

    auto params = rtac::zero<Params>();
    params.width  = W;
    params.height = H;
    params.output = output.data();
    params.cam    = helpers::PinholeCamera::Create(float3({0.0f,0.0f,0.0f}),
                                                float3({5.0f,4.0f,3.0f}));
    //params.topObject = *cube0;
    //params.topObject = *topObject;
    params.topObject = *topTopObject;
    
    CUdeviceptr dparams;
    cudaMalloc((void**)&dparams, sizeof(Params));
    cudaMemcpy((void*)dparams, &params, sizeof(Params), cudaMemcpyHostToDevice);
    
    OPTIX_CHECK( optixLaunch(*pipeline, 0,
                             dparams, sizeof(Params),
                             &sbt, W, H, 1) );
    cudaDeviceSynchronize();

    //print_output(W,H,output);

    //HostVector<unsigned char> data(output);
    //rtac::files::write_pgm("output.pgm", W, H, (const char*)data.data());
    HostVector<uchar3> data(output);
    rtac::files::write_ppm("output.ppm", W, H, (const char*)data.data());

    return 0;
}

template<typename T>
void print_output(unsigned int W, unsigned int H, const DeviceVector<T>& output)
{
    HostVector<T> data(output);
    for(int h = 0; h < H; h++) {
        for(int w = 0; w < W; w++) {
            cout << " " << (unsigned int)data[W*h + w];
        }
        cout << endl;
    }
}
