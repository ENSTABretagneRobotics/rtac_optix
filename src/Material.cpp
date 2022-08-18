#include <rtac_optix/Material.h>

namespace rtac { namespace optix {

MaterialBase::MaterialBase(unsigned int rayTypeIndex, const ProgramGroup::Ptr& hitPrograms) :
    ShaderBindingBase(hitPrograms),
    rayTypeIndex_(rayTypeIndex)
{}

unsigned int MaterialBase::raytype_index() const
{
    return rayTypeIndex_;
}

}; //namespace optix
}; //namespace rtac
