#ifndef _DEF_RTAC_OPTIX_GROUP_INSTANCE_H_
#define _DEF_RTAC_OPTIX_GROUP_INSTANCE_H_

#include <memory>
#include <iostream>
#include <cstring>
#include <array>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/Instance.h>
#include <rtac_optix/InstanceAccelStruct.h>

namespace rtac { namespace optix {

class GroupInstance : public Instance
{
    public:

    using Ptr       = std::shared_ptr<GroupInstance>;
    using ConstPtr  = std::shared_ptr<const GroupInstance>;
    using Instances = InstanceAccelStruct::Instances;

    protected:

    InstanceAccelStruct::Ptr instanceAS_;

    GroupInstance(const Context::ConstPtr& context,
                  const InstanceAccelStruct::Ptr& instanceAS);

    public:

    static Ptr Create(const Context::ConstPtr& context);

    void add_instance(const Instance::Ptr& instance);
    const Instances& instances() const;
    InstanceAccelStruct::ConstPtr instanceAS() const;
};

}; //namespace optix
}; //namespace rtac


#endif //_DEF_RTAC_OPTIX_GROUP_INSTANCE_H_
