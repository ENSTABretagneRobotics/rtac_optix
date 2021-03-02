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
using namespace rtac::optix;

#include <rtac_optix_7_autosbt_test/ptx_files.h>

#include "autosbt_test.h"

void visit_graph(Instance::Ptr top, unsigned int level = 0)
{
    switch(top->build_type()) {
        case OPTIX_BUILD_INPUT_TYPE_TRIANGLES:
        case OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES:
            cout << level << ", " << top.get() << " is an object" << endl;
            break;
        case OPTIX_BUILD_INPUT_TYPE_INSTANCES:
        case OPTIX_BUILD_INPUT_TYPE_INSTANCE_POINTERS:
            cout << level << ", " << top.get() << " is a group" << endl;
            for(auto& instance : std::dynamic_pointer_cast<GroupInstance>(top)->instances()) {
                visit_graph(instance, level + 1);
            }
            break;
        case OPTIX_BUILD_INPUT_TYPE_CURVES:
            throw std::runtime_error("Got curves, not implemented yet");
            break;
        default:
            throw std::runtime_error("Unknown build type");
            break;
    }
}

int main()
{
    //static_assert(false, "Build a scene with lights and shadows");
    //static_assert(false, "Add miss materials");
    auto ptxFiles = rtac_optix_7_autosbt_test::get_ptx_files();

    optix_init();
    auto context = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("module0", ptxFiles["src/autosbt_test.cu"]);

    auto raygen  = pipeline->add_raygen_program("__raygen__autosbt_test", "module0");
    auto rgbMiss = pipeline->add_miss_program("__miss__autosbt_rgb", "module0");
    auto shadowMiss = pipeline->add_miss_program("__miss__autosbt_shadow", "module0");
    
    auto hitRgbDesc = zero<OptixProgramGroupDesc>();
    hitRgbDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitRgbDesc.hitgroup.moduleCH            = pipeline->module("module0");
    hitRgbDesc.hitgroup.entryFunctionNameCH = "__closesthit__autosbt_rgb";
    auto hitRgb = pipeline->add_program_group(hitRgbDesc);

    auto hitShadowDesc = zero<OptixProgramGroupDesc>();
    hitShadowDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitShadowDesc.hitgroup.moduleCH            = pipeline->module("module0");
    hitShadowDesc.hitgroup.entryFunctionNameCH = "__closesthit__autosbt_shadow";
    auto hitShadow = pipeline->add_program_group(hitShadowDesc);

    pipeline->link();
    
    auto cubeGeom = MeshGeometry::CreateCube(context);
    cubeGeom->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE});
    
    auto yellow = Material<RgbRay, RgbHitData>::Create(hitRgb, RgbHitData({uchar3({255,255,0})}));
    auto cyan   = Material<RgbRay, RgbHitData>::Create(hitRgb, RgbHitData({uchar3({0,255,255})}));
    auto majenta= Material<RgbRay, RgbHitData>::Create(hitRgb, RgbHitData({uchar3({255,0,255})}));

    std::cout << "record size : " << majenta->record_size() << endl;
    
    auto cube0 = ObjectInstance::Create(cubeGeom);
    cube0->add_material(yellow);
    cube0->set_transform({1,0,0,0,
                          0,1,0,0,
                          0,0,1,1});
    auto cube1 = ObjectInstance::Create(cubeGeom);
    //cube1->add_material(yellow);
    cube1->add_material(cyan);
    //cube1->add_material(majenta);
    cube1->set_transform({4,0,0,0,
                          0,4,0,0,
                          0,0,4,-4});

    auto topObject = GroupInstance::Create(context);
    topObject->add_instance(cube0);
    topObject->add_instance(cube1);

    //visit_graph(topObject);
    ShaderBindingTable<2> sbt;

    sbt.set_raygen_record(Material<RgbRay, SbtRecord<void>>::Create(
        raygen, SbtRecord<void>()));
    sbt.add_miss_record(Material<RgbRay, RgbMissData>::Create(
        rgbMiss, RgbMissData({uchar3({50,50,50})})));
    sbt.add_object(cube0);
    sbt.add_object(cube1);

    int W = 1024, H = 768;
    cuda::DeviceVector<uchar3> output(W*H);

    Params params;
    params.width  = W;
    params.height = H;
    params.output = output.data();
    params.cam = PinholeCamera::New({0,0,0}, {5,4,3});
    params.topObject = *topObject;
    //params.light = {4,2,10};

    OPTIX_CHECK( optixLaunch(*pipeline, 0, 
                             (CUdeviceptr)cuda::memcpy::host_to_device(params), sizeof(params),
                             sbt.sbt(), W, H, 1) );
    cudaDeviceSynchronize();
    CUDA_CHECK_LAST();

    cuda::HostVector<uchar3> imgData(output);
    files::write_ppm("output.ppm", W, H, (const char*) imgData.data());

    return 0;
}

