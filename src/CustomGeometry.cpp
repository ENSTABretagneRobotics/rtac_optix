#include <rtac_optix/CustomGeometry.h>

namespace rtac { namespace optix {

/**
 * Default OptixBuildInput for custom geometries. Only one primitive per
 * CustomGeometry is allowed for now.
 *
 * Default fields of OptixBuildBuild :
 * - type                          : OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES
 * - customPrimitive.numPrimitives : 1
 * 
 * @return a default empty OptixBuildInput.
 */
OptixBuildInput CustomGeometry::default_build_input()
{
    auto res = types::zero<OptixBuildInput>();
    res.type = OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES;
    res.customPrimitiveArray.numPrimitives = 1; // locking this to 1 for now.
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
OptixAccelBuildOptions CustomGeometry::default_build_options()
{
    return GeometryAccelStruct::default_build_options();
}

CustomGeometry::CustomGeometry(const Context::ConstPtr& context) :
    GeometryAccelStruct(context, default_build_input(), default_build_options())
{
    this->set_aabb({-1,-1,-1,1,1,1});
}

/**
 * Creates a new empty instance of CustomGeomtry.
 *
 * @param context a non-null Context pointer. The Context cannot be
 *                changed in the object lifetime.
 *
 * @return a shared pointer to the newly created CustomGeometry.
 */
CustomGeometry::Ptr CustomGeometry::Create(const Context::ConstPtr& context)
{
    return Ptr(new CustomGeometry(context));
}

/**
 * Sets Axis Aligned Bounding Box (aabb) of the geometry.
 *
 * This boundingbox is used by OptiX to trigger the call to a user-defined
 * \_\_intersection\_\_ program which itself checks if the ray intersected with
 * the CustomGeometry.
 * 
 * @param aabb a std::vector<float> of size 6 holding the Axis Aligned Bounding
 *             Box [xmin, xmax, ymin, ymax, zmin, zmax] for this
 *             CustomGeometry.
 */
void CustomGeometry::set_aabb(const std::vector<float>& aabb)
{
    aabb_ = aabb;
    if(this->geomData_.size() == 0)
        this->geomData_.resize(1);
    this->geomData_[0] = reinterpret_cast<CUdeviceptr>(aabb_.data());
    this->buildInput_.customPrimitiveArray.aabbBuffers = this->geomData_.data();
}

/**
 * @return the number of primitives defined in the OptixBuildInput (for now
 *         locked to only one in CustomGeometry).
 */
unsigned int CustomGeometry::primitive_count() const
{
    return this->buildInput_.customPrimitiveArray.numPrimitives;
}

}; //namespace optix
}; //namespace rtac
