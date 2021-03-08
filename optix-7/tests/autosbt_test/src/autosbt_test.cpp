#include <iostream>
using namespace std;

#include <rtac_base/files.h>
#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/HostVector.h>
using namespace rtac;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/MeshGeometry.h>
#include <rtac_optix/ObjectInstance.h>
//#include <rtac_optix/InstanceAccelStruct.h>
#include <rtac_optix/GroupInstance.h>
#include <rtac_optix/ShaderBindingTable.h>
#include <rtac_optix/ShaderBinding.h>
using namespace rtac::optix;

#include <rtac_optix_7_autosbt_test/ptx_files.h>

#include "autosbt_test.h"

int main()
{
    auto ptxFiles = rtac_optix_7_autosbt_test::get_ptx_files();

    optix_init();
    auto context = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("module0", ptxFiles["src/autosbt_test.cu"]);

    auto raygen  = pipeline->add_raygen_program("__raygen__autosbt_test", "module0");
    auto rgbMiss = pipeline->add_miss_program("__miss__autosbt_rgb", "module0");
    auto shadowMiss = pipeline->add_miss_program("__miss__autosbt_shadow", "module0");
    
    auto hitRgb = pipeline->add_hit_programs();
    hitRgb->set_closesthit({"__closesthit__autosbt_rgb", pipeline->module("module0")});

    auto hitShadow = pipeline->add_hit_programs();
    hitShadow->set_closesthit({"__closesthit__autosbt_shadow", pipeline->module("module0")});

    pipeline->link();
    
    auto cubeGeom = MeshGeometry::CreateCube(context);
    cubeGeom->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE},
                                 std::vector<uint8_t>({0,1,0,1,0,1,0,1,0,1,0,1}));
    cubeGeom->enable_vertex_access();

    float3 light = float3({-5,-2,7});
    auto yellow = RgbMaterial::Create(hitRgb, RgbHitData({uchar3({255,255,0}), light}));
    auto cyan   = RgbMaterial::Create(hitRgb, RgbHitData({uchar3({0,255,255}), light}));
    auto majenta= RgbMaterial::Create(hitRgb, RgbHitData({uchar3({255,0,255}), light}));

    auto cube0 = ObjectInstance::Create(cubeGeom);
    cube0->add_material(yellow,  0);
    cube0->add_material(majenta, 1);
    cube0->set_transform({1,0,0,0,
                          0,1,0,0,
                          0,0,1,1});
    auto cube1 = ObjectInstance::Create(cubeGeom);
    cube1->add_material(cyan,    0);
    cube1->add_material(majenta, 1);
    cube1->set_transform({4,0,0,0,
                          0,4,0,0,
                          0,0,4,-4});

    auto topObject = GroupInstance::Create(context);
    topObject->add_instance(cube0);
    topObject->add_instance(cube1);

    //visit_graph(topObject);
    auto sbt = ShaderBindingTable<2>::Create();

    sbt->set_raygen_record(ShaderBinding<void>::Create(raygen));
    sbt->add_miss_record(RgbMissMaterial::Create(rgbMiss,RgbMissData({uchar3({50,50,50})})));
    sbt->add_miss_record(ShadowMaterial::Create(shadowMiss));
    sbt->add_object(cube0);
    sbt->add_object(cube1);

    int W = 1024, H = 768;
    cuda::DeviceVector<uchar3> output(W*H);

    Params params;
    params.width  = W;
    params.height = H;
    params.output = output.data();
    params.cam = PinholeCamera::New({0,0,0}, {3,5,3});
    params.topObject = *topObject;
    //params.light = {4,2,10};

    OPTIX_CHECK( optixLaunch(*pipeline, 0, 
                             (CUdeviceptr)cuda::memcpy::host_to_device(params), sizeof(params),
                             sbt->sbt(), W, H, 1) );
    cudaDeviceSynchronize();
    CUDA_CHECK_LAST();

    cuda::HostVector<uchar3> imgData(output);
    files::write_ppm("output.ppm", W, H, (const char*) imgData.data());

    return 0;
}

