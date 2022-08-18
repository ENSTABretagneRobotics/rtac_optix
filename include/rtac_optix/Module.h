#ifndef _DEF_RTAC_OPTIX_MODULE_H_
#define _DEF_RTAC_OPTIX_MODULE_H_

#include <iostream>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>

namespace rtac { namespace optix {

// Forward declaration of class Pipeline (only pipeline will be allowed to
// Create a new Module
class Pipeline;

/**
 * A wrapper around the OptixModule type.
 *
 * A Module is the result of the compilation of a CUDA source file. It is the
 * equivalent of a C/C++ object file (.o). A single Module contains several
 * functions to be used in a ray-tracing program. Several Modules can be linked
 * together with a Pipeline to make a full OptiX ray-tracing executable code.
 *
 * A Module is used to create a ProgramGroup. A ProgramGroup is defined by at
 * least a pair containing a function name (a symbol) and a Module in which the
 * function is defined.
 *
 * The user cannot create a standalone Module (the Constructor and the Create
 * functions are protected). Instead, a Module must be created by calling the
 * Pipeline::add_module method from an existing Pipeline. This allows to tie
 * the Pipeline and its Modules together and free the user from managing shared
 * options.
 */
class Module : public OptixWrapper<OptixModule>
{
    public:

    friend class Pipeline;

    using Ptr      = OptixWrapperHandle<Module>;
    using ConstPtr = OptixWrapperHandle<const Module>;

    using PipelineOptions = OptixPipelineCompileOptions;
    using ModuleOptions   = OptixModuleCompileOptions;
    static ModuleOptions default_options();

    protected:
    
    Context::ConstPtr   context_;
    std::string         ptxSource_;
    PipelineOptions     pipelineOptions_;
    ModuleOptions       moduleOptions_;

    virtual void do_build() const;
    virtual void clean() const;

    Module(const Context::ConstPtr& context,
           const std::string& ptxSource,
           const PipelineOptions& pipelineOptions,
           const ModuleOptions& moduleOptions);

    static Ptr Create(const Context::ConstPtr& context,
                      const std::string& ptxSource,
                      const PipelineOptions& pipelineOptions,
                      const ModuleOptions& options = default_options());

    public:

    ~Module();

    const PipelineOptions& pipeline_options() const;
    const ModuleOptions&   module_options() const;
    // These getters will invalidate the pipeline which will automatically be
    // rebuilt on cast to OptixPipeline type.
    PipelineOptions& pipeline_options();
    ModuleOptions&   module_options();
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_MODULE_H_
