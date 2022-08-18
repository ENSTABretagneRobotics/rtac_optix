#include <rtac_optix/GeometryAccelStruct.h>

namespace rtac { namespace optix {

/**
 * @return a default (invalid and zeroed) OptixBuildInput.
 */
GeometryAccelStruct::BuildInput GeometryAccelStruct::default_build_input()
{
    return AccelerationStruct::default_build_input();
}

/**
 * Default options (same as AccelerationStruct::default_build_options) :
 * - buildFlags    : OPTIX_BUILD_FLAG_NONE
 * - operation     : OPTIX_BUILD_OPERATION_BUILD
 * - motionOptions : zeroed OptixMotionOptions struct
 * 
 * @return a default OptixAccelBuildOptions for a build operation.
 */
GeometryAccelStruct::BuildOptions GeometryAccelStruct::default_build_options()
{
    return AccelerationStruct::default_build_options();
}

/**
 * Default hit flag configuration (see GeometryAccelStruct::material_hit_setup).
 *
 * By default every primitive has the same material index (0).
 *
 * @return configuration for a single material with flag
 *         OPTIX_GEOMETRY_FLAG_NONE.
 */
std::vector<unsigned int> GeometryAccelStruct::default_hit_flags()
{
    return std::vector<unsigned int>({OPTIX_GEOMETRY_FLAG_NONE});
}

/**
 * Protected Constructor. Should be called by a sub-class Constructor.
 *
 * @param context      a non-null Context pointer. The Context cannot be
 *                     changed in the AccelerationStruct object lifetime.
 * @param buildInput   a OptixBuildInput instance. Can be modified after the
 *                     object instanciation. Usually provided by the sub-class
 *                     Constructor.
 * @param buildOptions a OptixAccelBuildOptions instance. Can be modified after
 *                     the object instanciation. Usually provided by the
 *                     sub-class Constructor.
 *
 * Loads a default material configuration (all primitives have the same
 * material index = 0).
 */
GeometryAccelStruct::GeometryAccelStruct(const Context::ConstPtr& context,
                                         const OptixBuildInput& buildInput,
                                         const OptixAccelBuildOptions& options) :
    AccelerationStruct(context, buildInput, options)
{
    this->material_hit_setup(default_hit_flags());
}

/**
 * Fill-in the OptixBuildInput buildInput_ from the material configuration in
 * materialHitFlags_ and materialIndexes_ attributes. This is called before the
 * build process in GeometryAccelStruct::do_build.
 */
void GeometryAccelStruct::update_hit_setup() const
{
    switch(this->buildInput_.type)
    {
        case OPTIX_BUILD_INPUT_TYPE_TRIANGLES:
            if(materialHitFlags_.size() > 0) {
                this->buildInput_.triangleArray.flags         = materialHitFlags_.data();
                this->buildInput_.triangleArray.numSbtRecords = materialHitFlags_.size();
            }
            else {
                this->buildInput_.triangleArray.flags         = nullptr;
                this->buildInput_.triangleArray.numSbtRecords = 0;
            }
            if(materialIndexes_) {
                if(materialIndexes_->size() != this->primitive_count()) {
                    std::ostringstream oss;
                    oss << "GeometryAccelStruct::material_hit_setup : "
                        << "Invalid number of material indexes, must be the same than "
                        << "the number of primitives (got " << materialIndexes_->size()
                        << ", expected " << this->primitive_count() << ")";
                    throw std::runtime_error(oss.str());
                }
                this->buildInput_.triangleArray.sbtIndexOffsetBuffer =
                    (CUdeviceptr)materialIndexes_->data();
                this->buildInput_.triangleArray.sbtIndexOffsetSizeInBytes   = 1;
                this->buildInput_.triangleArray.sbtIndexOffsetStrideInBytes = 1;
            }
            break;
        case OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES:
            if(materialHitFlags_.size() > 0) {
                this->buildInput_.customPrimitiveArray.flags         = materialHitFlags_.data();
                this->buildInput_.customPrimitiveArray.numSbtRecords = materialHitFlags_.size();
            }
            else {
                this->buildInput_.customPrimitiveArray.flags         = nullptr;
                this->buildInput_.customPrimitiveArray.numSbtRecords = 0;
            }
            break;
        case OPTIX_BUILD_INPUT_TYPE_CURVES:
            throw std::logic_error("Curves not implemented yet.");
            break;
        case OPTIX_BUILD_INPUT_TYPE_INSTANCES:
        case OPTIX_BUILD_INPUT_TYPE_INSTANCE_POINTERS:
            throw std::runtime_error(
                "Invalid buildInput type (Instances) for GeometryAccelStruct");
            break;
        default:
            throw std::logic_error("Fatal error : Unknown build type. Check OptiX version");
            break;
    };
}

/**
 * Creates the underlying OptixTraversableHandle.
 *
 * Calls GeometryAccelStruct::update_hit_setup, then creates the
 * OptixTraversableHandle from OptixBuildInput buildInput_ and
 * OptixAccelBuildOptions buildOptions_ by calling optixAccelBuild.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called when a user request to
 * OptixTraversableHandle occurs.
 */
void GeometryAccelStruct::do_build() const
{
    this->update_hit_setup();
    AccelerationStruct::do_build();
}

/**
 * Configures the **materials indexes** for the primitives of this
 * GeometryAccelStruct (associates a pair [index,flags] to each primitive).
 *
 * @param hitFlags        a std::vector with OptixGeometryFlags flags.
 *                        hitFlags.size() defines the number of different
 *                        materials for this GeometryAccelStruct. (and all
 *                        **material indexes** must be between 0 and
 *                        hitFlags.size()-1).
 * @param materialIndexes a handle to a DeviceVector containing the **material
 *                        indexes** associated with each primitive. Values must
 *                        be between 0 and hitFlags.size()-1.
 *                        materialIndexes.size() must be equal to the number of
 *                        primitives in the geometry. Can be null if there is
 *                        only one Material in the geometryAccelStruct (i.e. if
 *                        hitFlags.size() == 1)
 */
void GeometryAccelStruct::material_hit_setup(
    const std::vector<unsigned int>& hitFlags,
    const Handle<MaterialIndexBuffer>& materialIndexes)
{
    if(hitFlags.size() == 0) {
        this->bump_version();
        materialHitFlags_.resize(0);
    }
    else if(hitFlags.size() == 1) {
        if(materialIndexes) {
            std::cerr << "Warning, GeomAccelStruct : You provided a materialIndexes "
                      << "buffer but requested only one material type. "
                      << "The indexes will be ignored";
        }
        this->bump_version();
        materialHitFlags_ = hitFlags;
    }
    else if(hitFlags.size() > 1) {
        if(!materialIndexes) {
            std::ostringstream oss;
            oss << "GeometryAccelStruct::material_hit_setup : "
                << "several materials hit flags where provided (implying your"
                << "geometry has several materials), but no material index "
                << "buffer was provided (buffer size must be number of "
                << "primitives in the geometry, buffers value must be a "
                << "material index).";
            throw std::runtime_error(oss.str());
        }
        if(materialIndexes->size() != this->primitive_count()) {
            std::ostringstream oss;
            oss << "GeometryAccelStruct::material_hit_setup : "
                << "Invalid number of material indexes, must be the same than "
                << "the number of primitives (got " << materialIndexes->size()
                << ", expected " << this->primitive_count() << ")";
            throw std::runtime_error(oss.str());
        }
        this->bump_version();
        materialHitFlags_ = hitFlags;
        materialIndexes_  = materialIndexes;
    }
}

/**
 * Overload of GeometryAccelStruct::material_hit_setup with a host-side
 * material indexes vector.
 */
void GeometryAccelStruct::material_hit_setup(
    const std::vector<unsigned int>& hitFlags,
    const std::vector<uint8_t>& materialIndexes)
{
    this->material_hit_setup(hitFlags,
        Handle<MaterialIndexBuffer>(new MaterialIndexBuffer(materialIndexes)));
}

/**
 * Clears and invalidate the current material configuration.
 *
 * Trying to build/use this object without reconfiguring the materials will
 * result in an error.
 */
void GeometryAccelStruct::clear_hit_setup()
{
    this->bump_version();
    materialHitFlags_.resize(0);
}

/**
 * @return the width this object takes in the ShaderBindingTable. (Without
 *         taking into account the number of ray types. It is equal to the
 *         number of different materials in this GeometryAccelStruct.
 *
 * See ShaderBindingTable for more information.
 */
unsigned int GeometryAccelStruct::sbt_width() const
{
    return materialHitFlags_.size();
}

}; //namespace optix
}; //namespace rtac
