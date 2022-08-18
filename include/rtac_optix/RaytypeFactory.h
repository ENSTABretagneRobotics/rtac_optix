#ifndef _DEF_RTAC_OPTIX_RAYTYPE_FACTORY_H_
#define _DEF_RTAC_OPTIX_RAYTYPE_FACTORY_H_

#include <tuple>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/Raytype.h>

namespace rtac { namespace optix {

template <class... RayPayloadTs>
class RaytypeFactory
{
    // This is a class intended to generate the Raytypes to be used both on
    // host-side on device-side functions.

    // Raytypes get unique ids which are used when tracing a ray in device-side
    // functions. These ids are also used on host  to generate the SBT (Shader
    // Binding Table). Similarly, the total number on Raytypes is used on both
    // side. This class together with Raytype and Material related type allow
    // to free the user from these considerations.
    
    public:

    using PayloadTypes = std::tuple<RayPayloadTs...>;
    static constexpr uint8_t RaytypeCount = sizeof...(RayPayloadTs);
    
    // This allows to associate a unique index to each Payload/Raytype.
    template <uint8_t Index, uint8_t MissSbtOffset = Index>
    using Raytype = Raytype<typename std::tuple_element<Index, PayloadTypes>::type,
                            Index, RaytypeCount, MissSbtOffset>;
};

}; //namespace optix
}; //namespace rtac


#endif //_DEF_RTAC_OPTIX_RAYTYPE_FACTORY_H_
