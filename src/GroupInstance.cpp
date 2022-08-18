#include <rtac_optix/GroupInstance.h>

namespace rtac { namespace optix {

GroupInstance::GroupInstance(const Context::ConstPtr& context,
                             const InstanceAccelStruct::Ptr& instanceAS) :
    Instance(instanceAS),
    instanceAS_(instanceAS)
{}

GroupInstance::Ptr GroupInstance::Create(const Context::ConstPtr& context)
{
    return Ptr(new GroupInstance(context, InstanceAccelStruct::Create(context)));
}

void GroupInstance::add_instance(const Instance::Ptr& instance)
{
    instanceAS_->add_instance(instance);
}

const GroupInstance::Instances& GroupInstance::instances() const
{
    return instanceAS_->instances();
}

InstanceAccelStruct::ConstPtr GroupInstance::instanceAS() const
{
    return instanceAS_;
}

}; //namespace optix
}; //namespace rtac
