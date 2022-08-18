#include <rtac_optix/Module.h>

namespace rtac { namespace optix {

/**
 * Generates default OptixModuleCompileOptions. Will be called on
 * Pipeline::add_module if no Module compile option were provided by the user.
 *
 * This provides a "to go" default configuration for a Module. These options
 * can be left as-is, but tweaking these options may lead to better
 * performances.
 *
 * Default options :
 * - maxRegisterCount : OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT
 * - optLevel         : OPTIX_COMPILE_OPTIMIZATION_DEFAULT
 * - debugLevel       : OPTIX_COMPILE_DEBUG_LEVEL_LINEINFO
 * - boundValues      : nullptr (0)
 * - numBoundValues   : 0
 *
 * @return an OptixModuleCompileOptions filled with default parameters.
 */
Module::ModuleOptions Module::default_options()
{
    auto options = types::zero<ModuleOptions>();

    options.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    options.optLevel         = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    options.debugLevel       = OPTIX_COMPILE_DEBUG_LEVEL_LINEINFO;

    return options;
}

Module::Module(const Context::ConstPtr& context,
               const std::string& ptxSource,
               const Module::PipelineOptions& pipelineOptions,
               const Module::ModuleOptions& moduleOptions) :
    context_(context),
    ptxSource_(ptxSource),
    pipelineOptions_(pipelineOptions),
    moduleOptions_(moduleOptions)
{}

/**
 * Instanciate a new Module. Is protected. Should not be called directly by the
 * user but with Pipeline::add_module method.
 *
 * @param context         a non-null Context pointer. The Context cannot be
 *                        changed in the Module object lifetime.
 * @param ptxSource       ptx source string. Output of the compilation of a
 *                        single CUDA source file with either NVCC (compile
 *                        time compilation) or NVRTC (runtime compilation).
 * @param pipelineOptions Pipeline compile options for this Module and its
 *                        Pipeline.  Defaults are provided by
 *                        Pipeline::default_compile_options. May be modified
 *                        after Module creation.
 * @param modulesOptions  compile option for this Module. Defaults are provided
 *                        by Module::default_options. May be modified after
 *                        Module creation.
 *
 * @return a shared pointer to the newly instanciated Module.
 */
Module::Ptr Module::Create(const Context::ConstPtr& context,
                           const std::string& ptxSource,
                           const Module::PipelineOptions& pipelineOptions,
                           const Module::ModuleOptions& moduleOptions)
{
    return Ptr(new Module(context, ptxSource, pipelineOptions, moduleOptions));
}

Module::~Module()
{
    try {
        this->clean();
    }
    catch(const std::exception& e) {
        std::cerr << "Caught exception during rtac::optix::Module destruction : " 
                  << e.what() << std::endl;
    }
}


/**
 * Effectively creates the underlying native Module type OptixModule using the
 * OptiX API functions.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called by the Module::build method when an
 * access to the OptiX native type OptixModule is requested.
 */
void Module::do_build() const
{
    OPTIX_CHECK( 
    optixModuleCreateFromPTX(*context_,
        &moduleOptions_, &pipelineOptions_,
        ptxSource_.c_str(), ptxSource_.size(),
        nullptr, nullptr, // These are logging related, log will also be
                          // written in context log, but with less tracking
                          // information (TODO Fix this. See a unified
                          // interface with Context type ?)
        &optixObject_
        ) );
}

/**
 * Destroy the underlying OptixModule Object (optixObject_)
 */
void Module::clean() const
{
    if(optixObject_)
        OPTIX_CHECK( optixModuleDestroy(optixObject_) );
}

const Module::PipelineOptions& Module::pipeline_options() const
{
    return pipelineOptions_;
}

const Module::ModuleOptions& Module::module_options() const
{
    return moduleOptions_;
}

/**
 * This will trigger a rebuild of the Module and its Pipeline (and all other
 * Modules and ProgramsGroups in the Pipeline).
 *
 * A writable access to pipelineOptions_ indicates that the user changed some
 * options and that the Pipeline and its Modules must be rebuilt with the new
 * options.
 *
 * @return a writable reference to compileOptions_. 
 */
Module::PipelineOptions& Module::pipeline_options()
{
    this->bump_version();
    return pipelineOptions_;
}

/**
 * This will trigger a rebuild of the Module and its depending ProgramGroups.
 *
 * A writable access to moduleOptions_ indicate that the user changed some
 * options and that the Module must be rebuilt with the new options.
 *
 * @return a writable reference to moduleOptions_. 
 */
Module::ModuleOptions& Module::module_options()
{
    this->bump_version();
    return moduleOptions_;
}

}; //namespace optix
}; //namespace rtac

