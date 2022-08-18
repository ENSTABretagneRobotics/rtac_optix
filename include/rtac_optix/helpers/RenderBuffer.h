#ifndef _DEF_RTAC_OPTIX_HELPERS_RENDER_BUFFER_H_
#define _DEF_RTAC_OPTIX_HELPERS_RENDER_BUFFER_H_

#include <vector>

#include <optix.h>

#include <rtac_base/type_utils.h>
#include <rtac_base/cuda/utils.h>

#include <rtac_optix/utils.h>

namespace rtac { namespace optix { namespace helpers {

template <typename T>
struct RenderBuffer
{
    protected:

    T*       data_;
    uint64_t size_;
    uint3    dims_;

    void allocate(uint64_t size);
    void free();
    
    public:

    static RenderBuffer<T> Create(unsigned int xsize,
                                  unsigned int ysize = 1,
                                  unsigned int zsize = 1);
    static RenderBuffer<T> Create(uint3 dims = {0,0,0});

    void resize(uint3 dims);

    void to_host(T* dst) const;
    std::vector<T> to_host() const;

    RTAC_HOSTDEVICE uint64_t size() const;
    RTAC_HOSTDEVICE uint3 dims() const;
    RTAC_HOSTDEVICE T& operator[](uint3 idx);
    RTAC_HOSTDEVICE const T& operator[](uint3 idx) const;

    RTAC_HOSTDEVICE T* begin();
    RTAC_HOSTDEVICE T* end();
    RTAC_HOSTDEVICE const T* begin() const;
    RTAC_HOSTDEVICE const T* end() const;
};

template <typename T>
RenderBuffer<T> RenderBuffer<T>::Create(unsigned int xsize,
                                        unsigned int ysize,
                                        unsigned int zsize)
{
    return RenderBuffer<T>::Create({xsize, ysize, zsize});
}

template <typename T>
RenderBuffer<T> RenderBuffer<T>::Create(uint3 dims)
{
    auto res = types::zero<RenderBuffer<T>>();
    res.resize(dims);
    return res;
}

template <typename T>
void RenderBuffer<T>::allocate(uint64_t size)
{
    if(size <= this->size())
        return;
    std::cout << "allocating " << size << std::endl;
    this->free();
    CUDA_CHECK( cudaMalloc(&data_, sizeof(T)*size) );
    size_ = size;
}

template <typename T>
void RenderBuffer<T>::free()
{
    if(data_)
        CUDA_CHECK( cudaFree(data_) );
}

template <typename T>
void RenderBuffer<T>::resize(uint3 dims)
{
    this->allocate(dims.x * dims.y * dims.z);
    dims_ = dims;
}

template <typename T>
void RenderBuffer<T>::to_host(T* dst) const
{
    CUDA_CHECK( cudaMemcpy(dst, data_, sizeof(T)*this->size(),
                           cudaMemcpyDeviceToHost) );
}

template <typename T>
std::vector<T> RenderBuffer<T>::to_host() const
{
    std::vector<T> res(this->size());
    this->to_host(res.data());
    return res;
}

template <typename T> RTAC_HOSTDEVICE
uint64_t RenderBuffer<T>::size() const
{
    return size_;
}

template <typename T> RTAC_HOSTDEVICE
uint3 RenderBuffer<T>::dims() const
{
    return dims_;
}

template <typename T> RTAC_HOSTDEVICE
T& RenderBuffer<T>::operator[](uint3 idx)
{
    return data_[dims_.z*(dims_.y*idx.x + idx.y) + idx.z];
}

template <typename T> RTAC_HOSTDEVICE
const T& RenderBuffer<T>::operator[](uint3 idx) const
{
    return data_[dims_.z*(dims_.y*idx.x + idx.y) + idx.z];
}

template <typename T> RTAC_HOSTDEVICE
T* RenderBuffer<T>::begin()
{
    return data_;
}

template <typename T> RTAC_HOSTDEVICE
T* RenderBuffer<T>::end()
{
    return data_ + this->size();
}

template <typename T> RTAC_HOSTDEVICE
const T* RenderBuffer<T>::begin() const
{
    return data_;
}

template <typename T> RTAC_HOSTDEVICE
const T* RenderBuffer<T>::end() const
{
    return data_ + this->size();
}

}; //namespace helpers
}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_HELPERS_RENDER_BUFFER_H_
