#include <iostream>
using namespace std;

#include <rtac_base/type_utils.h>
#include <rtac_base/files.h>
#include <rtac_base/cuda/DeviceObject.h>
using namespace rtac;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/MeshGeometry.h>
#include <rtac_optix/ObjectInstance.h>
#include <rtac_optix/GroupInstance.h>
#include <rtac_optix/ShaderBindingTable.h>
using namespace rtac::optix;

#include <helpers_rtac_optix/ptx_files.h>
#include "helpers.h"

int main()
{
    auto ptxFiles = helpers_rtac_optix::get_ptx_files();
    
    optix_init();
    auto context  = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("module0", ptxFiles["src/helpers.cu"]);

    auto raygen = pipeline->add_raygen_program("__raygen__helpers", "module0");

    auto rgbMiss    = pipeline->add_miss_program("__miss__helpers_rgb", "module0");
    auto shadowMiss = pipeline->add_miss_program("__miss__helpers_shadow", "module0");

    auto rgbHit = pipeline->add_hit_programs();
    rgbHit->set_closesthit({"__closesthit__helpers_rgb", pipeline->module("module0")});

    auto gray    = RGBMaterial::Create(rgbHit, RGBHitData({0.2,0.2,0.2}));
    auto majenta = RGBMaterial::Create(rgbHit, RGBHitData({1,0,1}));
    auto yellow  = RGBMaterial::Create(rgbHit, RGBHitData({1,1,0}));
    auto cyan    = RGBMaterial::Create(rgbHit, RGBHitData({0,1,1}));

    auto cubeGeom = MeshGeometry::CreateCube(context);
    std::vector<unsigned char> materialMap({0,0,0,0,1,1,2,2,1,1,2,2});
    cubeGeom->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE,
                                  OPTIX_GEOMETRY_FLAG_NONE,
                                  OPTIX_GEOMETRY_FLAG_NONE},
                                  materialMap);

    auto cube0 = ObjectInstance::Create(cubeGeom);
    cube0->add_material(majenta, 0);
    cube0->add_material(yellow,  1);
    cube0->add_material(cyan,    2);
    //cube0->set_position({0,0,1});

    auto topObject = GroupInstance::Create(context);
    topObject->add_instance(cube0);

    auto sbt = ShaderBindingTable::Create(Raytypes::RaytypeCount);
    sbt->set_raygen_program(raygen);
    sbt->add_miss_record(gray);
    sbt->add_miss_record(ShadowMaterial::Create(shadowMiss));
    sbt->add_object(cube0);

    unsigned int W = 800, H = 600;

    cuda::DeviceObject<Params> params;
    params.output    = RenderBuffer<float3>::Create(H, W);
    params.cam       = PinholeCamera::Create({0,0,0}, {5,4,3});
    params.topObject = *topObject;
    params.update_device();
    
    cout << params.output.size() << endl;
    cout << params.output.dims().x << " "
         << params.output.dims().y << " "
         << params.output.dims().z << endl;
    
    OPTIX_CHECK( optixLaunch(*pipeline, 0,
                             (CUdeviceptr)params.device_ptr(), sizeof(Params),
                             sbt->sbt(), H, W, 1) );
    cudaDeviceSynchronize();
    CUDA_CHECK_LAST();

    auto output = params.output.to_host();
    files::write_ppm("output.ppm", W, H, (const float*)output.data());

    return 0;
}
