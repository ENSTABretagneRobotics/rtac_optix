#ifndef _DEF_RTAC_OPTIX_UTILS_H_
#define _DEF_RTAC_OPTIX_UTILS_H_

#include <iostream>
#include <cstring>
#include <sstream>
#include <array>

#include <cuda_runtime.h>
#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/type_utils.h>
#include <rtac_base/cuda/utils.h>

#define OPTIX_CHECK( call )                                             \
    do {                                                                \
        OptixResult res = call;                                         \
        if(res != OPTIX_SUCCESS) {                                      \
            std::ostringstream oss;                                     \
            oss << "OptiX call '" << #call << "' failed '"              \
                << optixGetErrorName(res) << "' (code:" << res << ")\n" \
                << __FILE__ << ":" << __LINE__ << "\n";                 \
            throw std::runtime_error(oss.str());                        \
        }                                                               \
    } while(0)                                                          \


namespace rtac { namespace optix {

inline void optix_init()
{
    CUDA_CHECK( cudaFree(0) ); // Will initialize CUDA if not already done. 
                               // No-op if CUDA already initialized.
    OPTIX_CHECK( optixInit() );
}

inline unsigned int compute_aligned_offset(unsigned int size,
                                           unsigned int bytesAlignment)
{
    // This function computes the lowest number divisible by bytesAlignement
    // but greater than size. This allows to get a pointer aligned on a memory
    // space.
    return bytesAlignment * ((size + bytesAlignment - 1) / bytesAlignment);
}

template <unsigned int NumSizes, typename T = unsigned int>
std::array<T,NumSizes> compute_aligned_offsets(const std::array<T,NumSizes>& sizes,
                                                unsigned int bytesAlignment)
{
    // same as above but will accumulate offsets (for several pointers on a
    // contigous memory space).
    std::array<T,NumSizes> res;
    for(int i = 0, currentOffset = 0; i < NumSizes; i++) {
        res[i] = currentOffset + compute_aligned_offset(sizes[i], bytesAlignment);
        currentOffset = res[i];
    }
    return res;
}


template <typename T>
struct SbtRecord
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;

    SbtRecord() : data(types::zero<T>()) {}
    SbtRecord(const T& d) : data(d) {}
};

template <>
struct SbtRecord <void>
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_UTILS_H_
