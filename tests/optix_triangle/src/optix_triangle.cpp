//
// Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <array>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

#include <cuda_runtime.h>

#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stack_size.h>
#include <optix_stubs.h>

#include <rtac_base/cuda/DeviceVector.h>
#include <rtac_base/files.h>

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Pipeline.h>

#include <optix_triangle_rtac_optix/ptx_files.h>
#include "optix_triangle.h"

#include <rtac_optix/utils.h>

void OPTIX_CHECK_LOG(const OptixResult& res)
{
    if( res != OPTIX_SUCCESS ) {
        throw std::runtime_error("Got optix error");
    }
}


template <typename T>
struct SbtRecord
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

typedef SbtRecord<RayGenData>     RayGenSbtRecord;
typedef SbtRecord<MissData>       MissSbtRecord;
typedef SbtRecord<HitGroupData>   HitGroupSbtRecord;


static void context_log_cb( unsigned int level, const char* tag, const char* message, void* /*cbdata */)
{
    std::cerr << "[" << std::setw( 2 ) << level << "][" << std::setw( 12 ) << tag << "]: "
              << message << "\n";
}


int main( int argc, char* argv[] )
{
    std::string outfile;
    int         width  = 1024;
    int         height =  768;

    char log[2048]; // For error reporting from OptiX creation functions

    // Initialize CUDA
    CUDA_CHECK( cudaFree( 0 ) );
    // Initialize the OptiX API, loading all API entry points
    OPTIX_CHECK( optixInit() );
    auto context = rtac::optix::Context::Create();

    //
    // accel handling
    //
    OptixTraversableHandle gas_handle;
    CUdeviceptr            d_gas_output_buffer;
    {
        // Use default options for simplicity.  In a real use case we would want to
        // enable compaction, etc
        OptixAccelBuildOptions accel_options = {};
        accel_options.buildFlags = OPTIX_BUILD_FLAG_NONE;
        accel_options.operation  = OPTIX_BUILD_OPERATION_BUILD;

        // Triangle build input: simple list of three vertices
        const std::array<float3, 3> vertices =
        { {
              { -0.5f, -0.5f, 0.0f },
              {  0.5f, -0.5f, 0.0f },
              {  0.0f,  0.5f, 0.0f }
        } };

        const size_t vertices_size = sizeof( float3 )*vertices.size();
        CUdeviceptr d_vertices=0;
        CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &d_vertices ), vertices_size ) );
        CUDA_CHECK( cudaMemcpy(
                    reinterpret_cast<void*>( d_vertices ),
                    vertices.data(),
                    vertices_size,
                    cudaMemcpyHostToDevice
                    ) );

        // Our build input is a simple list of non-indexed triangle vertices
        const uint32_t triangle_input_flags[1] = { OPTIX_GEOMETRY_FLAG_NONE };
        OptixBuildInput triangle_input = {};
        triangle_input.type                        = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        triangle_input.triangleArray.vertexFormat  = OPTIX_VERTEX_FORMAT_FLOAT3;
        triangle_input.triangleArray.numVertices   = static_cast<uint32_t>( vertices.size() );
        triangle_input.triangleArray.vertexBuffers = &d_vertices;
        triangle_input.triangleArray.flags         = triangle_input_flags;
        triangle_input.triangleArray.numSbtRecords = 1;

        OptixAccelBufferSizes gas_buffer_sizes;
        OPTIX_CHECK( optixAccelComputeMemoryUsage(
                    *context,
                    &accel_options,
                    &triangle_input,
                    1, // Number of build inputs
                    &gas_buffer_sizes
                    ) );
        CUdeviceptr d_temp_buffer_gas;
        CUDA_CHECK( cudaMalloc(
                    reinterpret_cast<void**>( &d_temp_buffer_gas ),
                    gas_buffer_sizes.tempSizeInBytes
                    ) );
        CUDA_CHECK( cudaMalloc(
                    reinterpret_cast<void**>( &d_gas_output_buffer ),
                    gas_buffer_sizes.outputSizeInBytes
                    ) );

        OPTIX_CHECK( optixAccelBuild(
                    *context,
                    0,                  // CUDA stream
                    &accel_options,
                    &triangle_input,
                    1,                  // num build inputs
                    d_temp_buffer_gas,
                    gas_buffer_sizes.tempSizeInBytes,
                    d_gas_output_buffer,
                    gas_buffer_sizes.outputSizeInBytes,
                    &gas_handle,
                    nullptr,            // emitted property list
                    0                   // num emitted properties
                    ) );

        // We can now free the scratch space buffer used during build and the vertex
        // inputs, since they are not needed by our trivial shading method
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( d_temp_buffer_gas ) ) );
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( d_vertices        ) ) );
    }

    auto ptxFiles = optix_triangle_rtac_optix::get_ptx_files(); 
    auto pipeline0 = rtac::optix::Pipeline::Create(context);
    pipeline0->add_module("src/optix_triangle.cu", ptxFiles["src/optix_triangle.cu"]);

    auto raygen_prog_group = pipeline0->add_raygen_program("__raygen__rg", "src/optix_triangle.cu");
    auto miss_prog_group   = pipeline0->add_miss_program("__miss__ms", "src/optix_triangle.cu");

    //OptixProgramGroupDesc hitgroup_prog_group_desc = {};
    //hitgroup_prog_group_desc.kind                         = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    //hitgroup_prog_group_desc.hitgroup.moduleCH            = pipeline0->module("src/optix_triangle.cu");
    //hitgroup_prog_group_desc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
    //auto hitgroup_prog_group = pipeline0->add_program_group(hitgroup_prog_group_desc);
    auto hitgroup_prog_group = pipeline0->add_hit_programs();
    hitgroup_prog_group->set_closesthit({"__closesthit__ch", pipeline0->module("src/optix_triangle.cu")});

    //
    // Set up shader binding table
    //
    OptixShaderBindingTable sbt = {};
    {
        CUdeviceptr  raygen_record;
        const size_t raygen_record_size = sizeof( RayGenSbtRecord );
        CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &raygen_record ), raygen_record_size ) );
        RayGenSbtRecord rg_sbt;
        OPTIX_CHECK( optixSbtRecordPackHeader( *raygen_prog_group, &rg_sbt ) );
        CUDA_CHECK( cudaMemcpy(
                    reinterpret_cast<void*>( raygen_record ),
                    &rg_sbt,
                    raygen_record_size,
                    cudaMemcpyHostToDevice
                    ) );

        CUdeviceptr miss_record;
        size_t      miss_record_size = sizeof( MissSbtRecord );
        CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &miss_record ), miss_record_size ) );
        MissSbtRecord ms_sbt;
        ms_sbt.data = { 0.3f, 0.1f, 0.2f };
        OPTIX_CHECK( optixSbtRecordPackHeader( *miss_prog_group, &ms_sbt ) );
        CUDA_CHECK( cudaMemcpy(
                    reinterpret_cast<void*>( miss_record ),
                    &ms_sbt,
                    miss_record_size,
                    cudaMemcpyHostToDevice
                    ) );

        CUdeviceptr hitgroup_record;
        size_t      hitgroup_record_size = sizeof( HitGroupSbtRecord );
        CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &hitgroup_record ), hitgroup_record_size ) );
        HitGroupSbtRecord hg_sbt;
        OPTIX_CHECK( optixSbtRecordPackHeader( *hitgroup_prog_group, &hg_sbt ) );
        CUDA_CHECK( cudaMemcpy(
                    reinterpret_cast<void*>( hitgroup_record ),
                    &hg_sbt,
                    hitgroup_record_size,
                    cudaMemcpyHostToDevice
                    ) );

        sbt.raygenRecord                = raygen_record;
        sbt.missRecordBase              = miss_record;
        sbt.missRecordStrideInBytes     = sizeof( MissSbtRecord );
        sbt.missRecordCount             = 1;
        sbt.hitgroupRecordBase          = hitgroup_record;
        sbt.hitgroupRecordStrideInBytes = sizeof( HitGroupSbtRecord );
        sbt.hitgroupRecordCount         = 1;
    }


    //
    // launch
    //
    rtac::cuda::DeviceVector<uchar4> output_buffer(width*height); 
    {
        CUstream stream;
        CUDA_CHECK( cudaStreamCreate( &stream ) );

        //sutil::Camera cam;
        //configureCamera( cam, width, height );

        Params params;
        params.image        = output_buffer.data();
        params.image_width  = width;
        params.image_height = height;
        params.handle       = gas_handle;
        //params.cam_eye      = cam.eye();
        //cam.UVWFrame( params.cam_u, params.cam_v, params.cam_w );

        CUdeviceptr d_param;
        CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &d_param ), sizeof( Params ) ) );
        CUDA_CHECK( cudaMemcpy(
                    reinterpret_cast<void*>( d_param ),
                    &params, sizeof( params ),
                    cudaMemcpyHostToDevice
                    ) );

        OPTIX_CHECK( optixLaunch( *pipeline0, stream, d_param, sizeof( Params ), &sbt, width, height, /*depth=*/1 ) );
        cudaDeviceSynchronize();
        CUDA_CHECK(cudaGetLastError());
    }

    rtac::HostVector<uchar4> rgbaData(output_buffer);
    std::vector<char> rgbData(3*width*height);
    for(int i = 0, j = 0; i < rgbaData.size(); i++) {
        rgbData[j]     = rgbaData.data()[i].x;
        rgbData[j + 1] = rgbaData.data()[i].y;
        rgbData[j + 2] = rgbaData.data()[i].z;
        j +=3;
    }
    rtac::files::write_ppm("output.ppm", width, height, rgbData.data());

    {
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( sbt.raygenRecord       ) ) );
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( sbt.missRecordBase     ) ) );
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( sbt.hitgroupRecordBase ) ) );
        CUDA_CHECK( cudaFree( reinterpret_cast<void*>( d_gas_output_buffer    ) ) );
    }
    return 0;
}
