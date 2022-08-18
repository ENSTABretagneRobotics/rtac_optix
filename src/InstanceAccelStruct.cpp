#include <rtac_optix/InstanceAccelStruct.h>

namespace rtac { namespace optix {

/**
 * @return a default empty OptixBuildInput with type set to
 *         OPTIX_BUILD_INPUT_TYPE_INSTANCES.
 */
InstanceAccelStruct::BuildInput InstanceAccelStruct::default_build_input()
{
    auto res = types::zero<BuildInput>();
    res.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    return res;
}

/**
 * Default options (same as AccelerationStruct::default_build_options) :
 * - buildFlags    : OPTIX_BUILD_FLAG_NONE
 * - operation     : OPTIX_BUILD_OPERATION_BUILD
 * - motionOptions : zeroed OptixMotionOptions struct
 * 
 * @return a default OptixAccelBuildOptions for a build operation.
 */
InstanceAccelStruct::BuildOptions InstanceAccelStruct::default_build_options()
{
    return AccelerationStruct::default_build_options();
}

InstanceAccelStruct::InstanceAccelStruct(const Context::ConstPtr& context) :
    AccelerationStruct(context, default_build_input(), default_build_options())
{}

/**
 * Creates a new instance of InstanceAccelStruct.
 *
 * @param context a non-null Context pointer. The Context cannot be
 *                changed in the object lifetime.
 *
 * @return a shared pointer to the newly created InstanceAccelStruct.
 */
InstanceAccelStruct::Ptr InstanceAccelStruct::Create(const Context::ConstPtr& context)
{
    return Ptr(new InstanceAccelStruct(context));
}

/**
 * Creates the underlying OptixTraversableHandle.
 *
 * Updates the OptixBuildInput buildInput_ from the instances_ vector. Then,
 * builds the OptixTraversableHandle from OptixBuildInput buildInput_ and
 * OptixAccelBuildOptions buildOptions_ by calling optixAccelBuild.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called when a user request to
 * OptixTraversableHandle occurs.
 */
void InstanceAccelStruct::do_build() const
{
    // The buildInput_ needs to be updated before the build with created
    // instances.
    std::vector<OptixInstance> instances(instances_.size());
    for(int i = 0; i < instances.size(); i++) {
        instances[i] = *instances_[i];
    }
    tmpInstanceData_ = instances; // This uploads instances on the device.
    
    this->buildInput_.instanceArray.instances =
        reinterpret_cast<CUdeviceptr>(tmpInstanceData_.data());
    this->buildInput_.instanceArray.numInstances = tmpInstanceData_.size();

    this->AccelerationStruct::do_build();
}

/**
 * Add an existing Instance in the instances_ vector. The Instance is added to
 * the list of dependencies. (An update of instance will trigger a rebuild of
 * this InstanceAccelStruct).
 *
 * @param instance a pointer to an existing Instance (for the regular user it
 *                 would be an ObjectInstance or a GroupInstance).
 */
void InstanceAccelStruct::add_instance(const Instance::ConstPtr& instance)
{
    instances_.push_back(instance);
    this->add_dependency(instance);
}

const InstanceAccelStruct::Instances& InstanceAccelStruct::instances() const
{
    return instances_;
}

/**
 * @return the sum of all the sbt_width of currently managed Instance. See
 *        ShaderBindingTable for more information.
 */
unsigned int InstanceAccelStruct::sbt_width() const
{
    unsigned int res = 0;
    for(auto& instance : instances_) {
        res += instance->sbt_width();
    }
    return res;
}

}; //namespace optix
}; //namespace rtac
