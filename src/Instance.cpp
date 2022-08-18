#include <rtac_optix/Instance.h>

namespace rtac { namespace optix {

/**
 * Coefficients of the first three rows of a row-major 4x4 identity matrix.
 */
const float Instance::DefaultTransform[] = { 1.0f,0.0f,0.0f,0.0f,
                                             0.0f,1.0f,0.0f,0.0f,
                                             0.0f,0.0f,1.0f,0.0f };

/**
 * Generates default (valid, but without a TraversableHandle) OptixInstance
 * configuration.
 *
 * @param instanceId (defaults to 0) This is a user-set identifier for this
 *                   instance. In a **hitgroup** program, the instanceId of the
 *                   OptixInstance processed by the current ray can be queried
 *                   using optixGetInstanceId (For example this allows the user
 *                   to query specific information on the OptixInstance
 *                   directly from the ray **higroup** program). It does not
 *                   have to be unique to each Instance (can be left to 0 if
 *                   not used).
 *
 * Default values :
 * - transform         : Instance::DefaultTransform (Identity).
 * - instanceId        : 0
 * - sbtOffset         : 0
 * - visibilityMask    : 255
 * - flags             : Instance::DefaultFlags (OPTIX_INSTANCE_FLAG_NONE).
 * - traversableHandle : nullptr (0)
 */
OptixInstance Instance::default_instance(unsigned int instanceId)
{
    auto instance = types::zero<OptixInstance>();
    instance.visibilityMask = 255;
    instance.flags          = Instance::DefaultFlags;
    instance.instanceId     = instanceId;
    std::memcpy(&instance.transform,
                &Instance::DefaultTransform,
                12*sizeof(float));
    return instance;
}

Instance::Instance(const AccelerationStruct::ConstPtr& child,
                   unsigned int instanceId) :
    OptixWrapper<OptixInstance>(),
    child_(child)
{
    optixObject_ = default_instance(instanceId);
    if(!child) {
        throw std::runtime_error("Instance::Instance : the child of an instance "
            "should not be nullptr. It cannot be modified afterwards.");
    }
    this->add_dependency(child);
}

/**
 * Instanciates a new Instance.
 *
 * @param child a pointer to an AccelerationStruct (usually either a
 *                MeshGeometry, CustomGeometry or InstanceAccelStruct).
 * @param instanceId identifier for this Instance (defaults to 0). It is not
 *                   mandatory to have a unique identifier for each Instance.
 */
Instance::Ptr Instance::Create(const AccelerationStruct::ConstPtr& child,
                               unsigned int instanceId)
{
    return Ptr(new Instance(child, instanceId));
}

/**
 * Updates the OptixInstance with the OptixTraversableHandle in the child_
 * AccelerationStruct.
 *
 * There is no build operation per-say in the Instance class but this will
 * trigger the build of the child_ AccelerationStruct.
 */
void Instance::do_build() const
{
    if(this->child_)
        optixObject_.traversableHandle = *child_;
    else
        optixObject_.traversableHandle = 0;
}

/**
 * Sets the sbtOffset. Can be set by hand but the ShaderBindingTable type
 * should be used for that instead.
 *
 * In short : the sbtOffset can be viewed as a kind of function pointer. It is
 * used by the OptiX device API to retrieve the right **hitgroup** program to
 * be called when a ray enters the bounding box of the object Instance. The
 * calculation of the offset depends on a lot of factors.  See
 * ShaderBindingTable for more info.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::set_sbt_offset(unsigned int offset)
{
    optixObject_.sbtOffset = offset;
    this->bump_version(false);
}

/**
 * Sets the visibilityMask of this Instance. i
 *
 * The visibilityMask is used during the ray tracing operation. Each ray traced
 * with optixTrace has a ray visibilityMask which will be combined with the
 * OptixInstance visibilityMask.  If a binary AND between the masks gives 0,
 * the ray will ignore the OptixInstance.
 *
 * @param mask a 8-bits binary mask.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::set_visibility_mask(unsigned int mask)
{
    optixObject_.visibilityMask = mask;
    this->bump_version(false);
}

/**
 * Set the position of the Instance in 3D space.
 *
 * @param transform the coefficients for the first 3 rows of a row major
 *                  homogeneous matrix (the inverse of this transformation will
 *                  be applied to the ray during traversal of the object tree.
 *                  Any invertible transformation encoded in the matrix is valid).
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::set_transform(const std::array<float,12>& transform)
{
    std::memcpy(&optixObject_.transform, transform.data(), 12*sizeof(float));
    this->bump_version(false);
}

/**
 * Set the OptixInstance flags(overrides the current flags).
 *
 * @param flags a OptixInstanceFlags set of flags.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::set_flags(unsigned int flags)
{
    optixObject_.flags = flags;
    this->bump_version(false);
}

/**
 * Add a flag to the OptixInstance flags (performs a binary OR with the flags).
 *
 * @param flags a OptixInstanceFlags set of flags.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::add_flags(unsigned int flag)
{
    optixObject_.flags |= flag;
    this->bump_version(false);
}

/**
 * Remove a flag to the OptixInstance flags (performs a binary AND between the
 * already set flags and the binary inverse of the parameter flag).
 *
 * @param flags a OptixInstanceFlags set of flags.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::unset_flags(unsigned int flag)
{
    // invert all bits in flag then bitwise and will set flags to 0;
    optixObject_.flags &= ~flag; 
    this->bump_version(false);
}

unsigned int Instance::sbt_width() const
{
    if(!child_)
        return 0;
    return child_->sbt_width();
}

/**
 * @return the type of the child_ AccelerationStruct, OptixBuildInputType.
 */
OptixBuildInputType Instance::kind() const
{
    return child_->kind();
}

/**
 * @return the OptixTraversableHandle of the child_ AccelerationStruct. Will
 *         trigger the build of the child_ AccelerationStruct.
 */
Instance::operator const OptixTraversableHandle&() const
{
    if(!child_) {
        throw std::runtime_error("This instance has no child !");
    }
    return *child_;
}

// below here are only helpers / overrides
/**
 * Wrapper around Instance::set_transform to set only the translation of the Instance.
 *
 * @param pos 3D coordinates of the new Instance position.
 *
 * This won't trigger a rebuild of this Instance but will trigger a rebuild of
 * a dependent InstanceAccelStruct.
 */
void Instance::set_position(const std::array<float,3>& pos)
{
    optixObject_.transform[3]  = pos[0];
    optixObject_.transform[7]  = pos[1];
    optixObject_.transform[11] = pos[2];
    this->bump_version(false);
}

}; //namespace optix
}; //namespace rtac

std::ostream& operator<<(std::ostream& os, const OptixInstance& instance)
{
    const char* prefix = "\n- ";
    os << "OptixInstance :"
       << prefix << "instanceId        : " << instance.instanceId
       << prefix << "sbtOffset         : " << instance.sbtOffset
       << prefix << "visibilityMask    : " << instance.visibilityMask
       << prefix << "traversableHandle : " << instance.traversableHandle
       << prefix << "flags             : " << instance.flags
       << prefix << "transform         : ";
    for(int i = 0; i < 8; i += 4) {
        os << instance.transform[i]     << " ";
        os << instance.transform[i + 1] << " ";
        os << instance.transform[i + 2] << " ";
        os << instance.transform[i + 3];
        os << prefix << "                    ";
    }
    os << instance.transform[8]     << " ";
    os << instance.transform[8 + 1] << " ";
    os << instance.transform[8 + 2] << " ";
    os << instance.transform[8 + 3];
    return os;
}

