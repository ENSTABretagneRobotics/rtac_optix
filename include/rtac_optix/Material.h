#ifndef _DEF_RTAC_OPTIX_MATERIAL_H_
#define _DEF_RTAC_OPTIX_MATERIAL_H_

#include <iostream>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/ProgramGroup.h>
#include <rtac_optix/ShaderBinding.h>

namespace rtac { namespace optix {

class MaterialBase : public virtual ShaderBindingBase
{
    // Generic base Material class to be used in Instance type, but must not be
    // instanciated on its own.
    public:

    using Ptr      = OptixWrapperHandle<MaterialBase>;
    using ConstPtr = OptixWrapperHandle<const MaterialBase>;

    protected:

    unsigned int rayTypeIndex_;
    
    // have to be instanciated by a Material<> type
    MaterialBase(unsigned int rayTypeIndex, const ProgramGroup::Ptr& hitPrograms);

    public:

    unsigned int raytype_index() const;
};

template <typename RayT, class ParamsT>
class Material : public ShaderBinding<ParamsT>, public MaterialBase
{
    public:

    using RayType       = RayT;
    using ParamsType    = typename ShaderBinding<ParamsT>::ParamsType;
    using SbtRecordType = typename ShaderBinding<ParamsT>::SbtRecordType;

    using Ptr      = OptixWrapperHandle<Material<RayType, ParamsType>>;
    using ConstPtr = OptixWrapperHandle<const Material<RayType, ParamsType>>;
    
    protected:

    Material(const ProgramGroup::Ptr& hitPrograms, const SbtRecordType& params);

    public:

    static Ptr Create(const ProgramGroup::Ptr& hitPrograms,
                      const SbtRecordType& params = types::zero<SbtRecordType>());
};

template <typename RayT, class ParamsT>
Material<RayT,ParamsT>::Material(const ProgramGroup::Ptr& hitPrograms,
                                 const SbtRecordType& params) :
    ShaderBindingBase(hitPrograms),
    ShaderBinding<ParamsT>(hitPrograms, params),
    MaterialBase(RayT::Index, hitPrograms)
{}

template <typename RayT, class ParamsT>
typename Material<RayT,ParamsT>::Ptr Material<RayT,ParamsT>::Create(
    const ProgramGroup::Ptr& hitPrograms, const SbtRecordType& params)
{
    return Ptr(new Material<RayT,ParamsT>(hitPrograms, params));
}

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_MATERIAL_H_




