#ifndef _DEF_RTAC_OPTIX_7_TEST_COMPILE_TEST_H_
#define _DEF_RTAC_OPTIX_7_TEST_COMPILE_TEST_H_

#include <vector>

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

void copy(const thrust::device_vector<float>& input,
          thrust::device_vector<float>& output);
void copy(const std::vector<float>& input, std::vector<float>& output);

#endif //_DEF_RTAC_OPTIX_7_TEST_COMPILE_TEST_H_
