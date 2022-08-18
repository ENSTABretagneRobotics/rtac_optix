#ifndef _DEF_RTAC_OPTIX_RAYTYPE_H_
#define _DEF_RTAC_OPTIX_RAYTYPE_H_

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_base/cuda/utils.h>
#include <rtac_optix/helpers/maths.h>

namespace rtac { namespace optix {

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV = 1,
          uint8_t MissSbtOffsetV = SbtOffsetV>
struct Raytype : public PayloadT // Inheriting payload type for easy access
{
    // Base class for ray types. Intended to be used together with
    // RaytypeFactory to free the user from manually managing the 
    // ray / material / geometry indexes 

    // SbtOffsetV     is usually the RaytypeIndex
    // SbtStrideV     is usually the number of different Raytypes.
    // MissSbtOffsetV is to select the miss program (usually MissSbtOffset == SbtOffset)

    public:

    using PayloadType = PayloadT;
    static constexpr uint8_t Index     = SbtOffsetV;
    static constexpr uint8_t SbtStride = SbtStrideV;
    static constexpr uint8_t MissIndex = MissSbtOffsetV;

    // Mininal number of 4 bytes registers to hold PayloadType data.
    static constexpr uint8_t RegisterCount = (sizeof(PayloadType) + 3) / 4; 
    static constexpr uint8_t PaddingSize   = 4*RegisterCount - sizeof(PayloadType);

    // This padding is to ensure this struct size is aligned on uint32_t
    // boundaries.  This allows a generic copy to/from optix ray payload
    // registers.
    uint8_t pad__[PaddingSize];

    // Compile time check if no more than 8 payload registers are used.
    static_assert(RegisterCount <= 8,
        "Payload size too big for number of ray payload registers (max 8x4 bytes). "
        "If your payload is supposed to be small enough, check alignment");
    
    // No constructor to keep the aggregate behavior
    //RayPayload() : T() {}
    //RayPayload(const T& payload) : T(payload) {}

    #ifdef RTAC_CUDACC
    __device__ void trace(OptixTraversableHandle topObject,
                          const float3& rayOrigin,
                          const float3& rayDirection,
                          float tmin = 0.0f, float tmax = 1.0e8f,
                          float rayTime = 0.0f,
                          // Parameters below to be on ray type ?
                          OptixVisibilityMask visibilityMask = 255,
                          unsigned int rayFlags = OPTIX_RAY_FLAG_NONE);

    // return pointer to this cast to uint32_t
    __device__ const uint32_t* data() const;
    __device__       uint32_t* data();
    
    // copy from/to optix ray payload registers using optixSetPayload_n and optixGetPayload_n
    __device__ static void set_payload(const PayloadT& payload);
    __device__ void set_payload() const;

    __device__ void load_registers();
    __device__ static Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV> from_registers();

    __device__ static float3 world_origin();
    __device__ static float3 world_direction();
    __device__ static float3 current_world_position();

    __device__ static float3 object_origin();
    __device__ static float3 object_direction();
    __device__ static float3 current_object_position();

    __device__ static float tmin();
    __device__ static float tmax();
    #endif
};

#ifdef RTAC_CUDACC // Register getter/setters

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ void Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
trace(OptixTraversableHandle topObject,
      const float3& rayOrigin,
      const float3& rayDirection,
      float tmin, float tmax, float rayTime,
      // Parameters below to be on ray type ?
      OptixVisibilityMask visibilityMask,
      unsigned int rayFlags)
{
    if constexpr(RegisterCount == 0) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex);
    }
    if constexpr(RegisterCount == 1) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0]);
    }
    if constexpr(RegisterCount == 2) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1]);
    }
    if constexpr(RegisterCount == 3) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2]);
    }
    if constexpr(RegisterCount == 4) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2],
                   this->data()[3]);
    }
    if constexpr(RegisterCount == 5) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2],
                   this->data()[3],
                   this->data()[4]);
    }
    if constexpr(RegisterCount == 6) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2],
                   this->data()[3],
                   this->data()[4],
                   this->data()[5]);
    }
    if constexpr(RegisterCount == 7) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2],
                   this->data()[3],
                   this->data()[4],
                   this->data()[5],
                   this->data()[6]);
    }
    if constexpr(RegisterCount == 8) {
        optixTrace(topObject, rayOrigin, rayDirection, tmin, tmax, rayTime,
                   visibilityMask, rayFlags, Index, SbtStride, MissIndex,
                   this->data()[0],
                   this->data()[1],
                   this->data()[2],
                   this->data()[3],
                   this->data()[4],
                   this->data()[5],
                   this->data()[6],
                   this->data()[7]);
    }
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ const uint32_t* Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
data() const
{
    return reinterpret_cast<const uint32_t*>(this);
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ uint32_t* Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
data()
{
    return reinterpret_cast<uint32_t*>(this);
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ void Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
set_payload(const PayloadT& payload)
{
    // if constexpr is a c++17 feature which allows to ignore code at compile
    // time by checking the result of a constexpr test.
    auto data = reinterpret_cast<const uint32_t*>(&payload);
    if constexpr(RegisterCount > 0) optixSetPayload_0(data[0]);
    if constexpr(RegisterCount > 1) optixSetPayload_1(data[1]);
    if constexpr(RegisterCount > 2) optixSetPayload_2(data[2]);
    if constexpr(RegisterCount > 3) optixSetPayload_3(data[3]);
    if constexpr(RegisterCount > 4) optixSetPayload_4(data[4]);
    if constexpr(RegisterCount > 5) optixSetPayload_5(data[5]);
    if constexpr(RegisterCount > 6) optixSetPayload_6(data[6]);
    if constexpr(RegisterCount > 7) optixSetPayload_7(data[7]);
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ void Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
set_payload() const
{
    set_payload(*this);
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ void Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::
load_registers()
{
    // if constexpr is a c++17 feature which allows to ignore code at compile
    // time by checking the result of a constexpr test.
    if constexpr(RegisterCount > 0) this->data()[0] = optixGetPayload_0();
    if constexpr(RegisterCount > 1) this->data()[1] = optixGetPayload_1();
    if constexpr(RegisterCount > 2) this->data()[2] = optixGetPayload_2();
    if constexpr(RegisterCount > 3) this->data()[3] = optixGetPayload_3();
    if constexpr(RegisterCount > 4) this->data()[4] = optixGetPayload_4();
    if constexpr(RegisterCount > 5) this->data()[5] = optixGetPayload_5();
    if constexpr(RegisterCount > 6) this->data()[6] = optixGetPayload_6();
    if constexpr(RegisterCount > 7) this->data()[7] = optixGetPayload_7();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>
Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::from_registers()
{
    Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV> res;
    res.load_registers();
    return res;
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::world_origin()
{
    return optixGetWorldRayOrigin();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::world_direction()
{
    return optixGetWorldRayDirection();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::current_world_position()
{
    return optixGetWorldRayOrigin() + optixGetRayTmax() * optixGetWorldRayDirection();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::object_origin()
{
    return optixGetObjectRayOrigin();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::object_direction()
{
    return optixGetObjectRayDirection();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float3 Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::current_object_position()
{
    return optixGetObjectRayOrigin() + optixGetRayTmax() * optixGetObjectRayDirection();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::tmin()
{
    return optixGetRayTmin();
}

template <class PayloadT, uint8_t SbtOffsetV, uint8_t SbtStrideV, uint8_t MissSbtOffsetV>
__device__ float Raytype<PayloadT,SbtOffsetV,SbtStrideV,MissSbtOffsetV>::tmax()
{
    return optixGetRayTmax();
}


#endif //RTAC_CUDACC
}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_RAYTYPE_H_
