#include <rtac_optix/ShaderBinding.h>

namespace rtac { namespace optix {

ShaderBindingBase::ShaderBindingBase(const ProgramGroup::ConstPtr& program) :
    program_(program)
{}

ProgramGroup::ConstPtr ShaderBindingBase::program() const
{
    return program_;
}

}; //namespace optix
}; //namespace rtac
