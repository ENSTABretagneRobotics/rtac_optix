#ifndef _DEF_RTAC_OPTIX_PIPELINE_H_
#define _DEF_RTAC_OPTIX_PIPELINE_H_

#include <iostream>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/utils.h>
#include <rtac_optix/Handle.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/Module.h>
#include <rtac_optix/ProgramGroup.h>

namespace rtac { namespace optix {

/**
 * A wrapper around the OptixPipeline type.
 *
 * A Pipeline is the equivalent of a C/C++ build system for OptiX ray-tracing
 * programs. The Pipeline takes OptiX sources as inputs to build then into
 * Modules (see Module) which can contain several functions.  These Modules are
 * then used to create several ProgramGroups objects (see ProgramGroup). The
 * Pipeline then links the ProgramGroups together to build a fully-fledged
 * ray-tracing GPU program ready to be launched.
 *
 * In this API, the process of creating the Pipeline, a Module and a
 * ProgramGroup differs a bit from the official OptiX API process. In the
 * official OptiX API, modules and programs are created before the pipeline,
 * and the compileOptions and linkOptions used in Module creation and Pipeline
 * creation must be the same.  (In the official OptiX API, the call to
 * optixPipelineCreate is equivalent to a link). In this API, an instance of
 * Pipeline is the only allowed to create a new Module or a new ProgramGroup.
 * This allows to ensure that the compile and link options are the same across
 * all modules used in a particular Pipeline, and free the user from caring
 * about the compile and link options more than once.
 *
 * The Pipeline, Module and ProgramGroup classes are linked together by the
 * rtac::types::BuildTarget dependency system. Any change to a Module will
 * trigger the rebuild of a dependant ProgramGroup and subsequently of the
 * dependent Pipeline. This allows the user to change parameters of any object
 * without caring about keeping track of dependencies between objects.
 *
 * See the [rtac_optix_samples
 * repository](https://gitlab.ensta-bretagne.fr/narvorpi/rtac_optix_samples)
 * for examples of ray-tracing applications.
 */
class Pipeline : public OptixWrapper<OptixPipeline>
{
    public:

    using Ptr      = OptixWrapperHandle<Pipeline>;
    using ConstPtr = OptixWrapperHandle<const Pipeline>;

    using Modules  = std::unordered_map<std::string, Module::Ptr>;
    using Programs = std::vector<ProgramGroup::Ptr>;
    
    using CompileOptions = OptixPipelineCompileOptions;
    using LinkOptions    = OptixPipelineLinkOptions;
    static CompileOptions default_pipeline_compile_options();
    static LinkOptions    default_pipeline_link_options();

    protected:

    Context::ConstPtr context_;
    CompileOptions    compileOptions_;
    LinkOptions       linkOptions_;
    mutable bool      compileOptionsChanged_; 

    Modules  modules_;
    Programs programs_;

    void autoset_stack_sizes(); // not used anymore

    virtual void do_build() const;
    virtual void clean() const;

    Pipeline(const Context::ConstPtr& context,
             const CompileOptions& compileOptions,
             const LinkOptions& linkOptions);

    public:

    static Ptr Create(const Context::ConstPtr& context,
        const CompileOptions& compileOptions = default_pipeline_compile_options(),
        const LinkOptions&    linkOptions    = default_pipeline_link_options());
    ~Pipeline();

    virtual void build() const;

    const CompileOptions& compile_options() const;
    const LinkOptions&    link_options()    const;
    CompileOptions& compile_options();
    LinkOptions&    link_options();

    Module::Ptr add_module(const std::string& name, const std::string& ptxContent,
                           const Module::ModuleOptions& moduleOptions,
                           bool forceReplace = false);
    Module::Ptr      module(const std::string& name);
    Module::ConstPtr module(const std::string& name) const;
    bool contains_module(const Module::ConstPtr& module) const;

    ProgramGroup::Ptr add_program_group(ProgramGroup::Kind kind);

    // Starting from here, these are only overloads of already defined function
    // for convenience.
    Module::Ptr add_module(const std::string& name, const std::string& ptxContent,
                           bool forceReplace = false);

    ProgramGroup::Ptr add_raygen_program(const std::string& entryPoint,
                                         const std::string& moduleName);
    ProgramGroup::Ptr add_raygen_program(const std::string& entryPoint,
                                         const Module::Ptr& module);
    ProgramGroup::Ptr add_miss_program(const std::string& entryPoint,
                                       const std::string& moduleName);
    ProgramGroup::Ptr add_miss_program(const std::string& entryPoint,
                                       const Module::Ptr& module);
    ProgramGroup::Ptr add_hit_programs();
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_PIPELINE_H_
