#include <optix.h>
//#include <thrust/device_ptr.h>
//#include <thrust/host_vector.h>
//#include <thrust/device_vector.h>
#include <cuda_runtime.h>

#include "compile_test.h"

__device__
void device_copy(const float& v1, float& v2)
{
    v2 = v1;
}

__global__
void global_copy(const float* input, float* output)
{
    device_copy(input[threadIdx.x], output[threadIdx.x]);
}

//void copy(const thrust::device_vector<float>& input,
//          thrust::device_vector<float>& output)
//{
//    global_copy<<<1,input.size()>>>(thrust::raw_pointer_cast(input.data()),
//                                 thrust::raw_pointer_cast(output.data()));
//    cudaDeviceSynchronize();
//}

void copy(const std::vector<float>& input, std::vector<float>& output)
{
    //thrust::device_vector<float> in(input);
    //thrust::device_vector<float> out(in.size());
    //
    //copy(in, out);

    //output.resize(out.size());
    //cudaMemcpy(output.data(), thrust::raw_pointer_cast(out.data()),
    //           sizeof(float)*out.size(), cudaMemcpyDeviceToHost);

    float* inPtr;
    float* outPtr;

    cudaMalloc(&inPtr,  input.size()*sizeof(float));
    cudaMalloc(&outPtr, output.size()*sizeof(float));

    cudaMemcpy(inPtr, input.data(), input.size()*sizeof(float),
               cudaMemcpyHostToDevice);

    global_copy<<<1,input.size()>>>(inPtr, outPtr);
    cudaDeviceSynchronize();

    cudaMemcpy(output.data(), outPtr, output.size()*sizeof(float),
               cudaMemcpyDeviceToHost);
}




