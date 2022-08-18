#include <rtac_optix/Pipeline.h>

// This header must be included in the source file because it contains
// non-inlined function definitions (will trigger "multiple definition"
// errors).
// (Fixed in later version of optix. Current version is 7.1.0)
#include <optix_stack_size.h> // This is to auto-compute the stack size (difficult)

namespace rtac { namespace optix {

/**
 * Generates default OptixPipelineCompileOptions. Will be called on Pipeline
 * instanciation if no compile options were provided by the user.
 *
 * This provides a "to go" default configuration for a Pipeline. However some
 * options might have to be modified by the user (numPayloadValues and
 * numAttributesValue for example). Some other options can be left as-is in all
 * cases, but tweaking these options may lead to better performances.
 *
 * Default options :
 * - usesMotionBlur                   : false
 * - traversableGraphFlags            : OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY
 * - numPayloadValues                 : 3
 * - numAttributesValues              : 3
 * - exceptionFlags                   : OPTIX_EXCEPTION_FLAGS_NONE
 * - pipelineLaunchParamsVariableName : "params"
 * - usesPrimitiveTypeFlags           : OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE
 *
 * @return an OptixPipelineCompileOptions filled with default parameters.
 */
Pipeline::CompileOptions Pipeline::default_pipeline_compile_options()
{
    auto res = types::zero<CompileOptions>();

    res.usesMotionBlur        = false;
    //res.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    res.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
    res.numPayloadValues      = 3;
    res.numAttributeValues    = 3;
    // compileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_DEBUG
    //                               | OPTIX_EXCEPTION_FLAG_TRACE_DEPTH
    //                               | OPTIX_EXCEPTION_FLAG_STACK_OVERFLOW;
    res.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    res.pipelineLaunchParamsVariableName = "params";
    res.usesPrimitiveTypeFlags = OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE;

    return res;
}

/**
 * Generate default OptixPipelineLinkOptions. Will be called on Pipeline
 * instanciation if no link options were provided by the user.
 *
 * This provides a "to go" default configuration for a Pipeline. However some
 * options might have to be modified by the user (maxTraceDepth). Some other
 * options can be left as-is in all cases, but tweaking these options may lead
 * to better performances.
 *
 * Default options :
 * - maxTraceDepth : 1
 * - debugLevel    : OPTIX_COMPILE_DEBUG_LEVEL_FULL
 *
 * @return an OptixPipelineLinkOptions filled with default parameters.
 */
Pipeline::LinkOptions Pipeline::default_pipeline_link_options()
{
    auto res = types::zero<LinkOptions>();

    res.maxTraceDepth = 1;
    res.debugLevel    = OPTIX_COMPILE_DEBUG_LEVEL_FULL;

    return res;
}

Pipeline::Pipeline(const Context::ConstPtr& context,
                   const CompileOptions& compileOptions,
                   const LinkOptions& linkOptions) :
    context_(context),
    compileOptions_(compileOptions),
    linkOptions_(linkOptions),
    compileOptionsChanged_(false)
{}

/**
 * Instanciate a new Pipeline.
 *
 * @param context        a non-null Context pointer. The Context cannot be
 *                       changed in the Pipeline object lifetime.
 * @param compileOptions compile options for this Pipeline and its modules.
 *                       Defaults are provided by
 *                       Pipeline::default_compile_options. May be modified
 *                       after Pipeline creation.
 * @param linkOptions    link option for this Pipeline. Defaults are provided
 *                       by Pipeline::default_link_options. May be modified
 *                       after Pipeline creation.
 *
 * @return a shared pointer to the newly instanciated Pipeline.
 */
Pipeline::Ptr Pipeline::Create(const Context::ConstPtr& context,
                               const CompileOptions& compileOptions,
                               const LinkOptions& linkOptions)
{
    return Ptr(new Pipeline(context, compileOptions, linkOptions));
}

Pipeline::~Pipeline()
{
    try {
        this->clean();
    }
    catch(const std::runtime_error& e) {
        std::cerr << "Caught exception during rtac::optix::Pipeline destruction : " 
                  << e.what() << std::endl;
    }
}

/**
 * Overrides rtac::types::BuildTarget::build method to ensure dependent
 * modules (Module) are up to date with the Pipeline compileOptions_.
 *
 * Can be called by the user but there is not much point to it. It will be
 * automatically called on conversion to the native OptixPipline type (for
 * example on a ray-tracing launch).
 */
void Pipeline::build() const
{
    if(compileOptionsChanged_) {
        for(auto module : modules_) {
            module.second->pipeline_options() = compileOptions_;
        }
        compileOptionsChanged_ = false;
    }
    this->OptixWrapper<OptixPipeline>::build();
}

/**
 * Effectively creates the underlying native pipeline type OptixPipeline using
 * the OptiX API functions.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called by the Pipeline::build method when an
 * access to the OptiX native type OptixPipeline is requested.
 */
void Pipeline::do_build() const
{
    std::vector<OptixProgramGroup> compiledPrograms(programs_.size());
    for(int i = 0; i < programs_.size(); i++) {
        compiledPrograms[i] = *programs_[i];
    }

    OPTIX_CHECK(
    optixPipelineCreate(*context_, &compileOptions_, &linkOptions_,
        compiledPrograms.data(), compiledPrograms.size(), 
        nullptr, nullptr, // These are logging related, log will also
                          // be written in context log, but with less
                          // tracking information (TODO Fix this).
        &optixObject_));
}

/**
 * Destroy the underlying OptixPipeline Object (optixObject_)
 */
void Pipeline::clean() const
{
    if(optixObject_) {
        OPTIX_CHECK( optixPipelineDestroy(optixObject_) );
    }
}

/**
 * Not used anymore (OptiX do this by default, although it can be optimized by
 * the user). See
 * [here](https://raytracing-docs.nvidia.com/optix7/guide/index.html#program_pipeline_creation#pipeline-stack-size)
 * for more information.
 */
void Pipeline::autoset_stack_sizes()
{
    // Not used anymore

    // From the docs : defaults values will be set if none are given (if
    // maximum depth of call trees CC and DC programs is at most 2)

    // This is complicated... See the Optix 7 documentation.
    // Taken from the optix_triangle example in the OptiX-7.2 SDK
    OptixStackSizes stackSizes = {};
    for( auto& prog : programs_ ) {
        OPTIX_CHECK(optixUtilAccumulateStackSizes(*prog, &stackSizes));
    }
    
    uint32_t directCallableStackSizeFromTraversal;
    uint32_t directCallableStackSizeFromState;
    uint32_t continuationStackSize;
    OPTIX_CHECK(
    optixUtilComputeStackSizes(&stackSizes, linkOptions_.maxTraceDepth,
        0,  // maxCCDepth ?
        0,  // maxDCDEpth ?
        &directCallableStackSizeFromTraversal,
        &directCallableStackSizeFromState,
        &continuationStackSize));
    OPTIX_CHECK(
    optixPipelineSetStackSize( optixObject_,
        directCallableStackSizeFromTraversal,
        directCallableStackSizeFromState, continuationStackSize,
        1  // maxTraversableDepth ?
        ) );

}

const Pipeline::CompileOptions& Pipeline::compile_options() const
{
    return compileOptions_;
}

const Pipeline::LinkOptions& Pipeline::link_options() const
{
    return linkOptions_;
}

/**
 * This will trigger a rebuild of the Pipeline and its Modules (and the
 * associated ProgramGroups).
 *
 * A writable access to compileOptions_ indicate that the user changed some
 * options and that the Pipeline and its Module must be rebuilt with the new options.
 *
 * @return a writable reference to the compileOptions_. 
 */
Pipeline::CompileOptions& Pipeline::compile_options()
{
    this->bump_version();
    compileOptionsChanged_ = true;
    return compileOptions_;
}

/**
 * This will trigger a rebuild of the Pipeline.
 *
 * A writable access to linkOptions_ indicate that the user changed some
 * options and that the Pipeline must be rebuilt with the new options.
 *
 * @return a writable reference to the linkOptions_. 
 */
Pipeline::LinkOptions& Pipeline::link_options()
{
    this->bump_version();
    return linkOptions_;
}

/**
 * Creates a new Module object from a ptx source string and insert it into the
 * Pipeline module dictionary.
 *
 * The Module is added to the dependencies of this Pipeline.
 *
 * @param name          unique name for the Module (used to retrieve the module
 *                      using the Pipeline::module method).
 * @param ptxContent    a ptx source string which was compiled from CUDA code
 *                      using either NVCC at compile time or NVRTC at runtime (the
 *                      later is not implemented yet).
 * @param moduleOptions OptixModuleCompileOptions for this Module. Defaults are
 *                      given by Module::default_options().
 * @param forceReplace  if false, an error will be reported if the name was
 *                      already used to create a Module. Otherwise, the
 *                      existing Module with the same name will be removed (but
 *                      not destroyed. If an existing ProgramGroup still hold a
 *                      pointer to the removed Module, the removed Module
 *                      remains valid as long as the Pipeline compileOptions_
 *                      are not changed.
 *
 * @return a shared pointer to the newly created Module.
 */
Module::Ptr Pipeline::add_module(const std::string& name, const std::string& ptxContent,
                                 const Module::ModuleOptions& moduleOptions,
                                 bool forceReplace)
{
    // Checking if module already compiled.
    if(!forceReplace) {
        auto it = modules_.find(name);
        if(it != modules_.end()) {
            return it->second;
        }
    }
    
    this->bump_version();
    auto module = Module::Create(context_, ptxContent,
                                 compileOptions_, moduleOptions);
    modules_[name] = module;
    this->add_dependency(module);
    return module;
}

/**
 * Retrieve a Module given its name.
 * 
 * Throws a std::runtime_error if a Module with the given name was not found.
 *
 * @param name The name of the Module to be retrieved.
 *
 * @return a shared pointer to the newly created Module.
 */
Module::Ptr Pipeline::module(const std::string& name)
{
    auto it = modules_.find(name);
    if(it == modules_.end())
        throw std::runtime_error("No module with name '" + name + "'");
    return it->second;
}

/**
 * Retrieve a (const) Module given its name.
 * 
 * Throws a std::runtime_error if a Module with the given name was not found.
 *
 * @param name The name of the Module to be retrieved.
 *
 * @return a const shared pointer to the newly created Module.
 */
Module::ConstPtr Pipeline::module(const std::string& name) const
{
    auto it = modules_.find(name);
    if(it == modules_.end())
        throw std::runtime_error("No module with name '" + name + "'");
    return it->second;
}

bool Pipeline::contains_module(const Module::ConstPtr& module) const
{
    for(auto& item : modules_) {
        if(item.second == module)
            return true;
    }
    return false;
}

/**
 * Creates and return an empty ProgramGroup.
 *
 * The ProgramGroup is empty. It must be filled with one or several Function
 * information (function name and a module to be valid).
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * @param kind the ProgramGroup::Kind of ProgramGroup to create.
 *
 * @return a shared pointer to the newly created ProgramGroup instance.
 */
ProgramGroup::Ptr Pipeline::add_program_group(ProgramGroup::Kind kind)
{
    auto program = ProgramGroup::Create(context_, kind);
    programs_.push_back(program);
    this->add_dependency(program);
    return program;
}

// BELOW HERE, ONLY METHOD OVERLOADS
/**
 * Add a module with default OptixModuleCompileOptions.
 *
 * See Pipeline::add_module for more info.
 */
Module::Ptr Pipeline::add_module(const std::string& name, const std::string& ptxContent,
                                 bool forceReplace)
{
    return this->add_module(name, ptxContent,
                            Module::default_options(), forceReplace);
}

/**
 * Creates a new raygen ProgramGroup.
 *
 * The ProgramGroup is added to the Pipeline::programs_ list.
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * @param entryPoint name of the symbol (function name) of the entry function
 *                   in the module. Must start with __raygen__ prefix.
 * @param moduleName name of the Module which contains the function
 *                   "entryPoint". Should have already be added to this
 *                   Pipeline.
 *
 * @return a shared pointer to the newly created ProgramGroup.
 */
ProgramGroup::Ptr Pipeline::add_raygen_program(const std::string& entryPoint,
                                               const std::string& moduleName)
{
    auto program = this->add_program_group(OPTIX_PROGRAM_GROUP_KIND_RAYGEN);
    program->set_raygen({entryPoint, this->module(moduleName)});
    return program;
}

/**
 * Creates a new raygen ProgramGroup.
 *
 * The ProgramGroup is added to the Pipeline::programs_ list.
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * @param entryPoint name of the symbol (function name) of the entry function
 *                   in the module. Must start with __raygen__ prefix.
 * @param module     Module::Ptr which contains the function "entryPoint".
 *                   Should have already be added to this Pipeline.
 *
 * @return a shared pointer to the newly created ProgramGroup.
 */
ProgramGroup::Ptr Pipeline::add_raygen_program(const std::string& entryPoint,
                                               const Module::Ptr& module)
{
    if(!this->contains_module(module)) {
        throw std::runtime_error("Trying to create a program with a foreign module");
    }
    auto program = this->add_program_group(OPTIX_PROGRAM_GROUP_KIND_RAYGEN);
    program->set_raygen({entryPoint, module});
    return program;
}

/**
 * Creates a new miss ProgramGroup.
 *
 * The ProgramGroup is added to the Pipeline::programs_ list.
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * @param entryPoint name of the symbol (function name) of the entry function
 *                   in the module. Must start with the __miss__ prefix.
 * @param moduleName name of the Module which contains the function
 *                   "entryPoint". Should have already be added to this
 *                   Pipeline.
 *
 * @return a shared pointer to the newly created ProgramGroup.
 */
ProgramGroup::Ptr Pipeline::add_miss_program(const std::string& entryPoint,
                                             const std::string& moduleName)
{
    auto program = this->add_program_group(OPTIX_PROGRAM_GROUP_KIND_MISS);
    program->set_miss({entryPoint, this->module(moduleName)});
    return program;
}

/**
 * Creates a new miss ProgramGroup.
 *
 * The ProgramGroup is added to the Pipeline::programs_ list.
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * @param entryPoint name of the symbol (function name) of the entry function
 *                   in the module. Must start with __miss__ prefix.
 * @param module     Module::Ptr which contains the function "entryPoint".
 *                   Should have already be added to this Pipeline.
 *
 * @return a shared pointer to the newly created ProgramGroup.
 */
ProgramGroup::Ptr Pipeline::add_miss_program(const std::string& entryPoint,
                                             const Module::Ptr& module)
{
    if(!this->contains_module(module)) {
        throw std::runtime_error("Trying to create a program with a foreign module");
    }
    auto program = this->add_program_group(OPTIX_PROGRAM_GROUP_KIND_MISS);
    program->set_miss({entryPoint, module});
    return program;
}

/**
 * Creates an empty hitgroup ProgramGroup.
 *
 * The ProgramGroup is added to the Pipeline::programs_ list.
 *
 * The ProgramGroup is added to the dependencies of this Pipeline.
 *
 * The ProgramGroup::Function information should be added later by the user.
 *
 * @return a shared pointer to the newly created ProgramGroup.
 */
ProgramGroup::Ptr Pipeline::add_hit_programs()
{
    return this->add_program_group(OPTIX_PROGRAM_GROUP_KIND_HITGROUP);
}

}; //namespace optix
}; //namespace rtac





