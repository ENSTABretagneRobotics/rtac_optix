#include <rtac_optix/AccelerationStruct.h>

namespace rtac { namespace optix {

/**
 * Generates a default (invalid) OptixBuildInput
 *
 * @return a zeroed OptixBuildInput struct.
 */
AccelerationStruct::BuildInput AccelerationStruct::default_build_input()
{
    return types::zero<BuildInput>();
}

/**
 * Default options :
 * - buildFlags    : OPTIX_BUILD_FLAG_NONE
 * - operation     : OPTIX_BUILD_OPERATION_BUILD
 * - motionOptions : zeroed OptixMotionOptions struct
 * 
 * @return a default OptixAccelBuildOptions for a build operation.
 */
AccelerationStruct::BuildOptions AccelerationStruct::default_build_options()
{
    auto options = types::zero<BuildOptions>();
    options.buildFlags = OPTIX_BUILD_FLAG_NONE;
    options.operation  = OPTIX_BUILD_OPERATION_BUILD;
    return options;
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
 */
AccelerationStruct::AccelerationStruct(const Context::ConstPtr& context,
                                       const BuildInput& buildInput,
                                       const BuildOptions& buildOptions) :
    context_(context),
    buildInput_(buildInput),
    buildOptions_(buildOptions),
    buffer_(0),
    buildMeta_({Handle<Buffer>(nullptr), 0})
{}

/**
 * Creates the underlying OptixTraversableHandle.
 *
 * Creates the OptixTraversableHandle from OptixBuildInput buildInput_ and
 * OptixAccelBuildOptions buildOptions_ by calling optixAccelBuild.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called when a user request to
 * OptixTraversableHandle occurs.
 */
void AccelerationStruct::do_build() const
{
    // Computing memory usage needed(both for output and temporary usage for
    // the build itself) and resizing buffers accordingly;
    OptixAccelBufferSizes bufferSizes; // should I keep that in attributes ?
    OPTIX_CHECK(optixAccelComputeMemoryUsage(
        *context_, &buildOptions_, &buildInput_, 1, &bufferSizes) );

    if(!(buildOptions_.buildFlags & OPTIX_BUILD_FLAG_ALLOW_COMPACTION)) {
        // if compaction is not requested, building and exiting right away
        this->resize_build_buffer(bufferSizes.tempSizeInBytes);
        buffer_.resize(bufferSizes.outputSizeInBytes);
        OPTIX_CHECK(optixAccelBuild(*context_, buildMeta_.stream,
            &buildOptions_, &buildInput_, 1,
            reinterpret_cast<CUdeviceptr>(buildMeta_.buffer->data()), buildMeta_.buffer->size(),
            reinterpret_cast<CUdeviceptr>(buffer_.data()), buffer_.size(),
            &optixObject_, nullptr, 0));
    }
    else {
        // Compaction is requested, a second temporary space is needed.
        // using tempBuffer as a single temp memory space.
        // /!\ CATCH : memory space pointers must be 128bit aligned.
        // The begining of the second temporary space might not start directly
        // at the end of the first.
        // Also, output compacted size is returned by the build operation in
        // device memory, so an extra memory space must be reserved (size 64bits).

        // This function will compute the successive offsets at which the
        // output buffer and the compacted size may start.
        auto offsets = compute_aligned_offsets<2,size_t>(
            {bufferSizes.tempSizeInBytes, bufferSizes.outputSizeInBytes},
            128 / 8 );//128bits aligned

        // Compacted size will be returned as a uint64_t
        // (Total needed size is last offset + last size
        this->resize_build_buffer(offsets.back() + sizeof(uint64_t));
        
        // Building the request for the compacted size which will be send to
        // optixAccelBuild.
        auto propertyRequest   = types::zero<OptixAccelEmitDesc>();
        propertyRequest.type   = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
        propertyRequest.result = reinterpret_cast<CUdeviceptr>(
            buildMeta_.buffer->data() + offsets.back());
            
        OPTIX_CHECK(optixAccelBuild(*context_, buildMeta_.stream,
            &buildOptions_, &buildInput_, 1,
            reinterpret_cast<CUdeviceptr>(buildMeta_.buffer->data()), offsets[0],
            reinterpret_cast<CUdeviceptr>(buildMeta_.buffer->data() + offsets[0]),
            offsets[1] - offsets[0],
            &optixObject_, &propertyRequest, 1));

        // Retrieving compacted size
        uint64_t compactedSize;
        cudaMemcpy(&compactedSize, reinterpret_cast<const void*>(propertyRequest.result),
                   sizeof(compactedSize), cudaMemcpyDeviceToHost);
        
        // This check prevent the compaction if there is no gain to compact the
        // data.  However, in this implementation it will require to move the
        // data after completion if no compaction is possible and this feature
        // is not implemented yet. So the compaction executed in any case.
        //if(compactedSize < bufferSizes.outputSizeInBytes)
        {
            buffer_.resize(compactedSize);
            OPTIX_CHECK(optixAccelCompact(*context_, buildMeta_.stream, optixObject_,
                reinterpret_cast<CUdeviceptr>(buffer_.data()), buffer_.size(),
                &optixObject_));
        }
    }
}

const AccelerationStruct::BuildInput& AccelerationStruct::build_input() const
{
    return buildInput_;
}

const AccelerationStruct::BuildOptions& AccelerationStruct::build_options() const
{
    return buildOptions_;
}

/**
 * This will trigger a rebuild of the AccelerationStruct.
 *
 * A writable access to buildInput_ indicates that the user changed some
 * options and that the AccelerationStruct should be rebuilt with the new
 * options.
 *
 * @return a writable reference to OptixBuildInput buildInput_. 
 */
AccelerationStruct::BuildInput& AccelerationStruct::build_input()
{
    this->bump_version();
    return buildInput_;
}

/**
 * This will trigger a rebuild of the AccelerationStruct.
 *
 * A writable access to buildOptions_ indicates that the user changed some
 * options and that the AccelerationStruct should be rebuilt with the new
 * options.
 *
 * @return a writable reference to OptixAccelBuildOptions buildOptions_.
 */
AccelerationStruct::BuildOptions& AccelerationStruct::build_options()
{
    this->bump_version();
    return buildOptions_;
}

/**
 * Sets buffer for temporary buildData.
 *
 * If no temporary buffer was provided by the user a new one will be
 * automatically created. However, if the build operation of all the
 * AccelerationStruct in a project are performed sequentially, the same
 * temporary build buffer can be used across all the AccelerationStruct. This
 * should prevent multiple temporary buffer to be created on the device and
 * lead to reduce loading time and device memory used.
 *
 * @param buffer a shared pointer to a DeviceVector which will be used to hold
 *               temporary build data.
 */
void AccelerationStruct::set_build_buffer(const Handle<Buffer>& buffer)
{
    buildMeta_.buffer = buffer;
}

/**
 * Resizes the temporary build buffer according to the build process needs.
 */
void AccelerationStruct::resize_build_buffer(size_t size) const
{
    if(!buildMeta_.buffer) {
        buildMeta_.buffer = Handle<Buffer>(new Buffer(size));
    }
    else {
        buildMeta_.buffer->resize(size);
    }
}

/**
 * Sets a CUDA stream for parallel build of several AccelerationStruct. If none
 * is set by the user,the default CUstream (0) is used.
 *
 * Each AccelerationBuild to be built in parallel should run in its own CUDA
 * stream. Caution : each of the build thread should have its own temporary
 * build buffer (set with AccelerationStruct::set_build_buffer).
 *
 * @param stream a CUstream to run the build operation into.
 */
void AccelerationStruct::set_build_stream(CUstream stream)
{
    buildMeta_.stream = stream;
}

/**
 * Sets a temporary build buffer and a CUstream to run the build operation
 * into. See AccelerationStruct::set_build_buffer and
 * AccelerationStruct::set_build_stream for more details.
 *
 * @param buffer a shared pointer to a DeviceVector which will be used to hold
 *               temporary build data.
 * @param stream a CUstream to run the build operation into.
 */
void AccelerationStruct::set_build_meta(const Handle<Buffer>& buffer, CUstream stream)
{
    this->set_build_buffer(buffer);
    this->set_build_stream(stream);
}

/**
 * @return type of AccelerationStruct, OptixBuildInputType.
 */
OptixBuildInputType AccelerationStruct::kind() const
{
    return buildInput_.type;
}

}; //namespace optix
}; //namespace rtac
