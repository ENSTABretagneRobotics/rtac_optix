#include <iostream>
using namespace std;

#include <rtac_base/type_utils.h>
#include <rtac_base/files.h>
#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
using namespace rtac;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/MeshGeometry.h>
#include <rtac_optix/Material.h>
#include <rtac_optix/ObjectInstance.h>
#include <rtac_optix/InstanceAccelStruct.h>
using namespace rtac::optix;

#include <materials_test_rtac_optix/ptx_files.h>
#include "materials_test.h"

int main()
{
    auto ptxFiles = materials_test_rtac_optix::get_ptx_files();
    
    optix_init();
    auto context  = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("module0", ptxFiles["src/materials_test.cu"]);
    
    auto raygen = pipeline->add_raygen_program("__raygen__materials_test", "module0");
    auto miss   = pipeline->add_miss_program("__miss__materials_test", "module0");
    auto hitGroup = pipeline->add_hit_programs();
    hitGroup->set_closesthit({"__closesthit__materials_test", pipeline->module("module0")});

    auto cubeGeom = MeshGeometry::CreateCube(context); 
    std::vector<unsigned char> idxData(12);
    for(int i = 0; i < idxData.size(); i++) { idxData[i] = i & 0x1; }
    auto matIdx = std::shared_ptr<cuda::DeviceVector<unsigned char>>(
        new cuda::DeviceVector<unsigned char>(idxData));
    cubeGeom->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE},
                                 matIdx);

    auto yellow = Material<RGBRay, HitData>::Create(hitGroup, HitData({uchar3({255,255,0})}));
    auto blue   = Material<RGBRay, HitData>::Create(hitGroup, HitData({uchar3({0,0,255})}));

    auto cube0 = ObjectInstance::Create(cubeGeom);
    cube0->add_material(yellow, 0);
    //cube0->set_sbt_offset(1);
    cout << "cube0.sbt_width : " << cube0->sbt_width() << endl;

    auto topObject = InstanceAccelStruct::Create(context);
    topObject->add_instance(cube0);
    
    auto sbt = rtac::zero<OptixShaderBindingTable>();

    SbtRecord<RaygenData> raygenRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*raygen, &raygenRecord) );
    cudaMalloc((void**)&sbt.raygenRecord, sizeof(SbtRecord<RaygenData>));
    cudaMemcpy((void*)sbt.raygenRecord, &raygenRecord, sizeof(SbtRecord<RaygenData>),
               cudaMemcpyHostToDevice);

    SbtRecord<MissData> missRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*miss, &missRecord) );
    cudaMalloc((void**)&sbt.missRecordBase, sizeof(SbtRecord<MissData>));
    cudaMemcpy((void*)sbt.missRecordBase, &missRecord, sizeof(SbtRecord<MissData>),
               cudaMemcpyHostToDevice);
    sbt.missRecordCount         = 1;
    sbt.missRecordStrideInBytes = sizeof(SbtRecord<MissData>);

    std::vector<SbtRecord<HitData>> hitRecords(2);
    yellow->fill_sbt_record(&hitRecords[0]);
    blue->fill_sbt_record(&hitRecords[1]);
    cuda::DeviceVector<SbtRecord<HitData>> dHitRecords(hitRecords);
    sbt.hitgroupRecordBase          = (CUdeviceptr)dHitRecords.data();
    sbt.hitgroupRecordCount         = hitRecords.size();
    sbt.hitgroupRecordStrideInBytes = sizeof(SbtRecord<HitData>);
    
    int W = 1024, H = 768;
    cuda::DeviceVector<uchar3> output(W*H);
    Params params;
    params.width = W;
    params.height = H;
    params.imgData = output.data();
    params.cam = helpers::PinholeCamera::Create({0,0,0}, {5,4,3});
    params.topObject = *topObject;

    CUdeviceptr dparams;
    cudaMalloc((void**)&dparams, sizeof(Params));
    cudaMemcpy((void*)dparams, &params, sizeof(Params), cudaMemcpyHostToDevice);

    OPTIX_CHECK(optixLaunch(*pipeline, 0,
        dparams, sizeof(params),
        &sbt, W,H,1));
    cudaDeviceSynchronize();

    HostVector<uchar3> imgData(output);
    files::write_ppm("output.ppm",W,H,(const char*)imgData.data());

    return 0;

}
