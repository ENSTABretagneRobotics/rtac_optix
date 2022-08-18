#include <optix.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

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

void copy(const thrust::device_vector<float>& input,
          thrust::device_vector<float>& output)
{
    global_copy<<<1,input.size()>>>(thrust::raw_pointer_cast(input.data()),
                                 thrust::raw_pointer_cast(output.data()));
    cudaDeviceSynchronize();
}

void copy(const std::vector<float>& input, std::vector<float>& output)
{
    thrust::device_vector<float> in(input);
    thrust::device_vector<float> out(in.size());
    
    copy(in, out);

    output.resize(out.size());
    cudaMemcpy(output.data(), thrust::raw_pointer_cast(out.data()),
               sizeof(float)*out.size(), cudaMemcpyDeviceToHost);
}




