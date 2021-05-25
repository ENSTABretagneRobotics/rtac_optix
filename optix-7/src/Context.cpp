#include <rtac_optix/Context.h>
#include <optix_function_table_definition.h> // ?

namespace rtac { namespace optix {

/**
 * @return default options for context creation (logging and debug related).
 *         Logging level is set to 4 and OptixDeviceContextValidationMode is
 *         set to off.
 */
OptixDeviceContextOptions Context::default_options()
{
    OptixDeviceContextOptions res;
    std::memset(&res, 0, sizeof(res));

    res.logCallbackFunction = &Context::log_callback;
    res.logCallbackLevel    = 4;

    return res;
}

/**
 * Called by the OptiX API when logging.
 *
 * Outputs to stderr.
 */
void Context::log_callback( unsigned int level, const char* tag, const char* message, void* data)
{
    std::cerr << "[" << std::setw( 2 ) << level << "][" << std::setw( 12 ) << tag << "]: "
              << message << "\n";
}

Context::Context(const OptixDeviceContext& context) :
    context_(context)
{}

/**
 * Creates and initializes a new optix Context.
 *
 * @param options          OptixDeviceContextOptions. Logging related options.
 *                         See [here](https://raytracing-docs.nvidia.com/optix7/api/html/struct_optix_device_context_options.html)
 *                         for more information.
 * @param cudaContext      a cuda context (represent a specific GPU). Default is 0
 *                         (default GPU as seen by CUDA).
 * @param diskCacheEnabled whether to use the disk cache (disabled by default).
 */
Context::Ptr Context::Create(const OptixDeviceContextOptions& options,
                             CUcontext cudaContext,
                             bool diskCacheEnabled)
{
    optix_init();
    OptixDeviceContext context;
    OPTIX_CHECK( optixDeviceContextCreate(cudaContext, &options, &context) );

    auto res = Ptr(new Context(context));
    if(diskCacheEnabled)
        res->enable_cache();
    else
        res->disable_cache();
    return res;
}

Context::~Context()
{
    try {
        OPTIX_CHECK( optixDeviceContextDestroy(context_) );
    }
    catch(const std::runtime_error& e) {
        std::cerr << "Caught exception during rtac::optix::Context destruction : " 
                  << e.what() << std::endl;
    }
}

/**
 * Implicit conversion of this wrapper to OptixDeviceContext.
 *
 * This allows to directly use this object into OptiX API functions which take
 * an OptixDeviceContext object.
 */
Context::operator OptixDeviceContext() const
{
    return context_;
}

/**
 * Enables program cache (should speed-up application loading but not advised for debug).
 *
 * Cache is disabled by default.
 */
void Context::enable_cache()
{
    OPTIX_CHECK( optixDeviceContextSetCacheEnabled(context_, 1) );
}

/**
 * Disables program cache.
 *
 * Cache is disabled by default.
 */
void Context::disable_cache()
{
    OPTIX_CHECK( optixDeviceContextSetCacheEnabled(context_, 0) );
}

}; //namespace optix
}; //namespace rtac

