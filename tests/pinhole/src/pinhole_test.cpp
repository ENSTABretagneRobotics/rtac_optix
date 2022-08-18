#include <iostream>
using namespace std;

#include <cuda_runtime.h>
#include <optix.h>
#include <optix_stubs.h>

#include <rtac_base/type_utils.h>
#include <rtac_base/cuda/utils.h>
#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/cuda/HostVector.h>
using namespace rtac;
using namespace rtac::cuda;

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>
using namespace rtac::optix;

#include <rtac_optix/helpers/PinholeCamera.h>
using namespace rtac::optix::helpers;

#include <pinhole_test_rtac_optix/ptx_files.h>

#include "pinhole_test.h"

using RaygenRecord = SbtRecord<RaygenData>;
using MissRecord   = SbtRecord<MissData>;

int main()
{
    unsigned int W = 16, H = 9;

    auto ptxFiles = pinhole_test_rtac_optix::get_ptx_files();

    cudaFree(0);
    optixInit();

    auto context = Context::Create();
    auto pipeline = Pipeline::Create(context);
    pipeline->add_module("src/pinhole_test.cu", ptxFiles["src/pinhole_test.cu"]);
    auto raygenProgram = pipeline->add_raygen_program("__raygen__pinhole",
                                                     "src/pinhole_test.cu");
    // the missProgram is mandatory even without rays.
    auto missProgram = pipeline->add_miss_program("__miss__pinhole",
                                                 "src/pinhole_test.cu");

    // Shader binding table setup = setting program parameters
    auto sbt = types::zero<OptixShaderBindingTable>();
    // Is setting the record mandatory when empty ?
    RaygenRecord raygenRecord; // no parameters
    OPTIX_CHECK(optixSbtRecordPackHeader(*raygenProgram, &raygenRecord));
    //sbt.raygenRecord = reinterpret_cast<CUdeviceptr>(memcpy::host_to_device(raygenRecord));
    cudaMalloc((void**)&sbt.raygenRecord, sizeof(RaygenRecord));
    cudaMemcpy((void*)sbt.raygenRecord, &raygenRecord, sizeof(RaygenRecord), cudaMemcpyHostToDevice);
    MissRecord missRecord; // no parameters
    OPTIX_CHECK(optixSbtRecordPackHeader(*missProgram, &missRecord));
    cudaMalloc((void**)&sbt.missRecordBase, sizeof(MissRecord));
    cudaMemcpy((void*)sbt.missRecordBase, &missRecord, sizeof(MissRecord), cudaMemcpyHostToDevice);
    sbt.missRecordCount         = 1;
    sbt.missRecordStrideInBytes = sizeof(MissRecord);

    DeviceVector<float> imgData(W*H);

    Params params;
    params.width     = W;
    params.height    = H;
    params.imageData = imgData.data(); 
    params.cam = PinholeCamera::Create();
    cout << params.cam << endl;

    CUdeviceptr dparams;
    cudaMalloc((void**)&dparams, sizeof(Params));
    cudaMemcpy((void*)dparams, &params, sizeof(Params), cudaMemcpyHostToDevice);

    OPTIX_CHECK(optixLaunch(*pipeline, 0,
                            dparams, sizeof(params),
                            &sbt, W,H,1));
    cudaDeviceSynchronize();

    HostVector<float> output(imgData);
    for(int h = 0; h < H; h++) {
        for(int w = 0; w < W; w++) {
            cout << " " << output[h*W + w];
        }
        cout << endl;
    }

    return 0;
}

