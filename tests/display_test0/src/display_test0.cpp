#include <iostream>
using namespace std;

#include <rtac_base/type_utils.h>
#include <rtac_base/files.h>
using namespace rtac;
using namespace rtac::files;

#include <rtac_base/time.h>
using namespace rtac::time;

#include <rtac_base/types/common.h>
#include <rtac_base/types/Mesh.h>
using namespace rtac::types;

#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/Texture2D.h>
using namespace rtac::cuda;
using Texture = Texture2D<float4>;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
#include <rtac_optix/Instance.h>
#include <rtac_optix/InstanceAccelStruct.h>
#include <rtac_optix/MeshGeometry.h>
#include <rtac_optix/CustomGeometry.h>
using namespace rtac::optix;

#include <rtac_display/Display.h>
#include <rtac_display/GLVector.h>
#include <rtac_display/renderers/ImageRenderer.h>
using namespace rtac::display;

#include <display_test0_rtac_optix/ptx_files.h>

#include "display_test0.h"

template<>
struct rtac::display::GLFormat<uchar3> {

    using Scalar = unsigned char;

    static constexpr unsigned int Size  = 3;
    static constexpr GLenum PixelFormat = GL_RGB;
    static constexpr GLenum Type        = GL_UNSIGNED_BYTE;

    static constexpr GLenum InternalFormat = GL_RGB8;
};

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
    for(auto& f : cube->faces()) {
        auto p0 = cube->points()[f.x];
        auto p1 = cube->points()[f.y];
        auto p2 = cube->points()[f.z];
        
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
    //unsigned int W = 800, H = 600;
    unsigned int W = 1920, H = 1080;
    auto ptxFiles = display_test0_rtac_optix::get_ptx_files();

    rtac::cuda::init_cuda();
    OPTIX_CHECK( optixInit() );

    auto context  = Context::Create();

    // Building pipeline.
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("sbt_test0", ptxFiles["src/display_test0.cu"]);
    auto raygenProgram = pipeline->add_raygen_program("__raygen__sbt_test", "sbt_test0");
    auto missProgram   = pipeline->add_miss_program("__miss__sbt_test", "sbt_test0");

    auto hitProgram = pipeline->add_hit_programs();
    hitProgram->set_closesthit({"__closesthit__sbt_test", pipeline->module("sbt_test0")});

    auto sphereHitProgram = pipeline->add_hit_programs();
    sphereHitProgram->set_intersection({"__intersection__sphere", pipeline->module("sbt_test0")});
    sphereHitProgram->set_closesthit({"__closesthit__sphere", pipeline->module("sbt_test0")});

    // At this point the pipeline is not linked and the program are not
    // compiled yet. They will do so when used in an optix API call. (the
    // implicit cast between rtac::optix type and corresponding OptiX native
    // types will trigger compilation / link.
    
    // Creating scene
    
    // cubes as scene objects (sharing the same geometry acceleration structure).
    auto cubeMesh  = MeshGeometry::cube_data();
    auto cube = MeshGeometry::Create(context, cubeMesh);
    cube->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE});

    auto cubeInstance0 = Instance::Create(cube);
    auto cubeInstance1 = Instance::Create(cube);
    // Moving the second cube.
    cubeInstance1->set_transform({1.0f,0.0f,0.0f, -6.0f,
                                  0.0f,1.0f,0.0f, -1.0f,
                                  0.0f,0.0f,1.0f,  2.0f});
    // The sbt offset will allow to select a texture to be rendered on the cube.
    ///cubeInstance1->set_sbt_offset(sizeof(ClosestHitRecord)); // segfault.
    cubeInstance1->set_sbt_offset(1); // OK. Offset is in index, not in bytes.

    
    auto sphereAabb = CustomGeometry::Create(context);
    sphereAabb->material_hit_setup({OPTIX_GEOMETRY_FLAG_NONE});
    auto sphereInstance0 = Instance::Create(sphereAabb);
    sphereInstance0->set_sbt_offset(2); // OK. Offset is in index, not in bytes.
    sphereInstance0->set_transform({1.0f,0.0f,0.0f,  4.0f,
                                    0.0f,1.0f,0.0f, -2.0f,
                                    0.0f,0.0f,1.0f,  2.0f});
    
    auto topInstance = InstanceAccelStruct::Create(context);
    topInstance->add_instance(cubeInstance0);
    topInstance->add_instance(cubeInstance1);
    topInstance->add_instance(sphereInstance0);

    // Generating textures.
    auto checkerboardTex0 = Texture::checkerboard(16,16,
                                                  float4({1,1,0,1}),
                                                  float4({0,0,1,1}),
                                                  32);
    checkerboardTex0.set_filter_mode(Texture::FilterLinear);
    //checkerboardTex0.set_wrap_mode(Texture::WrapClamp);
    auto checkerboardTex1 = Texture::checkerboard(4,4,
                                                  float4({1,1,0,1}),
                                                  float4({1,0,0,1}),
                                                  1);
    checkerboardTex1.set_filter_mode(Texture::FilterLinear);
    auto uvBuffer = compute_cube_uv();
    
    // setting up sbt
    auto sbt = types::zero<OptixShaderBindingTable>();

    RaygenRecord raygenRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*raygenProgram, &raygenRecord) );
    cudaMalloc((void**)&sbt.raygenRecord, sizeof(RaygenRecord));
    cudaMemcpy((void*)sbt.raygenRecord, &raygenRecord, sizeof(RaygenRecord),
               cudaMemcpyHostToDevice);

    MissRecord missRecord;
    OPTIX_CHECK( optixSbtRecordPackHeader(*missProgram, &missRecord) );
    cudaMalloc((void**)&sbt.missRecordBase, sizeof(MissRecord));
    cudaMemcpy((void*)sbt.missRecordBase, &missRecord, sizeof(MissRecord),
               cudaMemcpyHostToDevice);
    sbt.missRecordCount = 1;
    sbt.missRecordStrideInBytes = sizeof(MissRecord);
    
    std::vector<ClosestHitRecord> hitRecordsHost(3);
    // hitrecord for cube 0
    hitRecordsHost[0].data.texObject     = checkerboardTex0;
    hitRecordsHost[0].data.cube.uvCoords = uvBuffer.data();
    OPTIX_CHECK( optixSbtRecordPackHeader(*hitProgram, &hitRecordsHost[0]) );
    // hitrecord for cube 1
    hitRecordsHost[1].data.texObject     = checkerboardTex1;
    hitRecordsHost[1].data.cube.uvCoords = uvBuffer.data();
    OPTIX_CHECK( optixSbtRecordPackHeader(*hitProgram, &hitRecordsHost[1]) );
    // hitrecord for sphere 0
    hitRecordsHost[2].data.texObject     = checkerboardTex0;
    //hitRecordsHost[2].data.texObject     = checkerboardTex1;
    hitRecordsHost[2].data.sphere.radius = 1.0f;
    OPTIX_CHECK( optixSbtRecordPackHeader(*sphereHitProgram, &hitRecordsHost[2]) );

    DeviceVector<ClosestHitRecord> hitRecords(hitRecordsHost);
    sbt.hitgroupRecordBase = reinterpret_cast<CUdeviceptr>(hitRecords.data());
    sbt.hitgroupRecordCount = 2;
    sbt.hitgroupRecordStrideInBytes = sizeof(ClosestHitRecord);

    //DeviceVector<uchar3> renderData(W*H); // Not needed (direct rendering to GL buffer)

    float3 camPos({5.0f,4.0f,3.0f});
    float3 camTarget({0.0f,0.0f,0.0f});
    auto params = types::zero<Params>();
    params.width     = W;
    params.height    = H;
    //params.imageData = renderData.data();
    params.imageData = nullptr;
    params.cam       = helpers::PinholeCamera::Create(camTarget, camPos);
    params.sceneTreeHandle = *topInstance;
    
    // Preparing display
    Display display;
    auto renderer = display.create_renderer<ImageRenderer>(View::New());

    GLVector<uchar3> imgData(W*H);

    FrameCounter counter;
    Clock clock;
    Clock clockRadius;
    while(!display.should_close()) {

        {
            // Updating camera position
            float dtheta = 0.5*clock.interval();
            float c = cos(dtheta), s = sin(dtheta);
            float2 newPos({c*camPos.x - s*camPos.y,
                           s*camPos.x + c*camPos.y});
            camPos.x = newPos.x;
            camPos.y = newPos.y;

            params.cam.look_at(camTarget, camPos);
            
            hitRecordsHost[2].data.sphere.radius = 1.0f + 0.5f*sin(3.0f*clockRadius.now());
            hitRecords = hitRecordsHost;
        }

        {
            // ptr must stay in scope for the duration of the render, it must
            // not be a temporary. However it must be destroyed before the use
            // of OpenGL to make the data available.
            // (This may work without unmapping the data, but the official
            // behavior is undefined. Use at your own risk !)
            auto ptr = imgData.map_cuda(); 
            params.imageData = ptr;

            CUdeviceptr dparams;
            cudaMalloc((void**)&dparams, sizeof(Params));
            cudaMemcpy((void*)dparams, &params, sizeof(Params), cudaMemcpyHostToDevice);

            // Updating ray-traced image (data in renderData
            OPTIX_CHECK( optixLaunch(*pipeline, 0, 
                                     dparams,
                                     sizeof(params), &sbt, W, H, 1) );
            cudaDeviceSynchronize(); // optixLaunch is asynchrounous
        }

        // Updating display
        //imgData = renderData;
        renderer->texture()->set_image(rtac::display::Shape({W,H}), imgData);

        display.draw();

        cout << counter;
    }

    //write_ppm("output.ppm", W, H,
    //          reinterpret_cast<const char*>(HostVector<uchar3>(imgData).data()));

    return 0;
}
