#include <rtac_optix/ProgramGroup.h>

namespace rtac { namespace optix {

const char* ProgramGroup::Function::Raygen               = "raygen";
const char* ProgramGroup::Function::Miss                 = "miss";
const char* ProgramGroup::Function::Exception            = "exception";
const char* ProgramGroup::Function::Intersection         = "intersection";
const char* ProgramGroup::Function::AnyHit               = "anyhit";
const char* ProgramGroup::Function::ClosestHit           = "closesthit";
const char* ProgramGroup::Function::DirectCallable       = "direct_callable";
const char* ProgramGroup::Function::ContinuationCallable = "continuation_callable";

/**
 * Creates an empty OptixProgramGroupDesc of a specific kind.
 *
 * @param kind can be any value amongst :
 * - OPTIX_PROGRAM_GROUP_KIND_RAYGEN
 * - OPTIX_PROGRAM_GROUP_KIND_MISS
 * - OPTIX_PROGRAM_GROUP_KIND_HITGROUP
 * - OPTIX_PROGRAM_GROUP_KIND_EXCEPTION
 * - OPTIX_PROGRAM_GROUP_KIND_CALLABLES
 * @param flags No ProgramGroup flags defined in OptiX for now. Default value
 * is : OPTIX_PROGRAM_GROUP_FLAGS_NONE
 *
 * @return an empty OptixProgramGroupDesc with valid king and flag fields.
 */
ProgramGroup::Description ProgramGroup::empty_description(const Kind& kind,  
                                                          unsigned int flags)
{
    auto description = types::zero<Description>();
    description.kind  = kind;
    description.flags = flags;
    return description;
}


/**
 * Generates default OptixProgramGroupOptions. It does not seemed to be used yet by
 * OptiX API (as of OptiX 7.3). It is probably there for next OptiX extensions.
 * 
 * Called by default when creating a new ProgramGroup.
 *
 * @return an zeroed OptixProgramGroupOptions instance.
 */
ProgramGroup::Options ProgramGroup::default_options()
{
    return types::zero<Options>();
}

ProgramGroup::ProgramGroup(const Context::ConstPtr& context,
                           Kind kind, unsigned int flags,
                           const Options& options) :
    context_(context),
    description_(empty_description(kind, flags)),
    options_(options)
{}

/**
 * Instanciate a new empty ProgramGroup. Is protected. Should not be called
 * directly by the user but with Pipeline::add_program_group or others Pipeline
 * ProgramGorup factory functions.
 *
 * @param context         a non-null Context pointer. The Context cannot be
 *                        changed in the ProgramGroup object lifetime.
 * @param kind            ProgramGroup::Kind
 * @param flags           Must be OPTIX_PROGRAM_GROUP_FLAGS_NONE on current
 *                        OptiX version (OptiX 7.3).
 * @param options         OptixProgramGroupOptions. Ignored on current OptiX
 *                        version (OptiX 7.3). Defaults to
 *                        ProgramGroup::default_options.
 *
 * @return a shared pointer to the newly instanciated ProgramGroup.
 */
ProgramGroup::Ptr ProgramGroup::Create(const Context::ConstPtr& context,
                                       Kind kind, unsigned int flags,
                                       const Options& options)
{
    return Ptr(new ProgramGroup(context, kind, flags, options));
}

ProgramGroup::~ProgramGroup()
{
    try {
        this->clean();
    }
    catch(const std::runtime_error& e) {
        std::cerr << "Caught exception during rtac::optix::ProgramGroup destruction : " 
                  << e.what() << std::endl;
    }
}

/**
 * Update the OptixProgramGroupDesc description_ using information from
 * functions_. ProgramGroup::update_description is called in
 * ProgramGroup::do_build for description_ to be used in the
 * optixProgramGroupCreate call.
 *
 * The rtac_optix workflow is based on a JIT (Just In Time) build paradigm.
 * Objects such as ProgramGroup, Module, Pipeline holds the parameters to build
 * their OptixProgramGroup, OptixModule and OptixPipeline counterparts. The
 * build operation itself consists in calling optixProgramGroupCreate (or
 * equivalent for other types) from the OptiX API to generate the native OptiX
 * types. This build operation is done **ONLY** when the native OptiX type is
 * explicitly needed, such in an OptiX API call.
 *
 * To follow this JIT paradigm, the OptixProgramGroupDesc must be filled just
 * before the ProgramGroup build because the OptixProgramGroupDesc holds some
 * references to OptixModule. If the OptixProgramGroupDesc was updated each
 * time a new Module is added, this would trigger the build of the Module when
 * it is converted to its OptixModule counterpart.
 */
void ProgramGroup::update_description() const
{
    auto f = functions_.end();
    switch(this->kind()) {
        case OPTIX_PROGRAM_GROUP_KIND_RAYGEN:
            f = this->function(Function::Raygen);
            description_.raygen.module            = *f->second.module;
            description_.raygen.entryFunctionName = f->second.name.c_str();
            break;
        case OPTIX_PROGRAM_GROUP_KIND_MISS:
            f = this->function(Function::Miss);
            description_.miss.module            = *f->second.module;
            description_.miss.entryFunctionName = f->second.name.c_str();
            break;
        case OPTIX_PROGRAM_GROUP_KIND_EXCEPTION:
            f = this->function(Function::Exception);
            description_.exception.module            = *f->second.module;
            description_.exception.entryFunctionName = f->second.name.c_str();
            break;
        case OPTIX_PROGRAM_GROUP_KIND_HITGROUP:
            try {
                f = this->function(Function::Intersection);
                description_.hitgroup.moduleIS            = *f->second.module;
                description_.hitgroup.entryFunctionNameIS = f->second.name.c_str();
            }
            catch(const FunctionNotFound&) {} // ignoring here but check f afterwards
            try {
                f = this->function(Function::AnyHit);
                description_.hitgroup.moduleAH            = *f->second.module;
                description_.hitgroup.entryFunctionNameAH = f->second.name.c_str();
            }
            catch(const FunctionNotFound&) {} // ignoring here but check f afterwards
            try {
                f = this->function(Function::ClosestHit);
                description_.hitgroup.moduleCH            = *f->second.module;
                description_.hitgroup.entryFunctionNameCH = f->second.name.c_str();
            }
            catch(const FunctionNotFound&) {} // ignoring here but check f afterwards
            if(f == functions_.end()) {
                // f not set, no function was successfully retrived through the
                // function method, so no program was set.
                std::ostringstream oss;
                oss << "ProgramGroup kind is HITGROUP but no "
                    << "intersection anyhit or closesthit function was set.";
                throw std::runtime_error(oss.str());
            }
            break;
        case OPTIX_PROGRAM_GROUP_KIND_CALLABLES:
            try {
                f = this->function(Function::DirectCallable);
                description_.callables.moduleDC            = *f->second.module;
                description_.callables.entryFunctionNameDC = f->second.name.c_str();
            }
            catch(const FunctionNotFound&) {} // ignoring here but check f afterwards
            try {
                f = this->function(Function::ContinuationCallable);
                description_.callables.moduleCC            = *f->second.module;
                description_.callables.entryFunctionNameCC = f->second.name.c_str();
            }
            catch(const FunctionNotFound&) {} // ignoring here but check f afterwards
            if(f == functions_.end()) {
                // f not set, no function was successfully retrived through the
                // function method, so no program was set.
                std::ostringstream oss;
                oss << "ProgramGroup kind is CALLABLES but no "
                    << "direct_callable or continuation_callable function was set.";
                throw std::runtime_error(oss.str());
            }
            break;
        default:
            throw std::runtime_error(
                "Unknown program group kind : check OptiX version");
            break;
    }
}

/**
 * Effectively creates the underlying native ProgramGroup type
 * OptixProgramGroup using the OptiX API functions.
 *
 * **DO NOT CALL THIS METHOD DIRECTLY UNLESS YOU KNOW WHAT YOU ARE DOING.**
 * This method will be automatically called by the ProgramGroup::build method
 * when an access to the OptiX native type OptixProgramGroup is requested.
 */
void ProgramGroup::do_build() const
{
    this->update_description();
    OPTIX_CHECK( optixProgramGroupCreate(
        *context_, &description_, 1, &options_,
        nullptr, nullptr, // These are logging related, log will also
                          // be written in context log, but with less
                          // tracking information (TODO Fix this).
        &optixObject_));
}

/**
 * Destroy the underlying OptixProgramGroup Object (optixObject_)
 */
void ProgramGroup::clean() const
{
    if(optixObject_ != nullptr)
        OPTIX_CHECK( optixProgramGroupDestroy(optixObject_) );
}

const ProgramGroup::Description& ProgramGroup::description() const
{
    return description_;
}

const ProgramGroup::Options& ProgramGroup::options() const
{
    return options_;
}

/**
 * This will trigger a rebuild of the ProgramGroup and its Pipeline.
 *
 * A writable access to description_ indicates that the user changed some
 * fields and that the Pipeline must be rebuilt with the updated values.
 *
 * @return a writable reference to description_. 
 */
ProgramGroup::Description& ProgramGroup::description()
{
    this->bump_version();
    return description_;
}

/**
 * This will trigger a rebuild of the ProgramGroup and its Pipeline.
 *
 * A writable access to description_ indicates that the user changed some
 * fields and that the Pipeline must be rebuilt with the updated values.
 *
 * @return a writable reference to options_. 
 */
ProgramGroup::Options& ProgramGroup::options()
{
    this->bump_version();
    return options_;
}

ProgramGroup::Kind ProgramGroup::kind() const
{
    return description_.kind;
}

unsigned int ProgramGroup::flags() const
{
    return description_.flags;
}

/**
 * Changes the ProgramGroup::Kind. Does nothing if already the requested kind.
 * Will trigger a rebuild if the kind changes.
 *
 * @param kind new ProgramGroup::Kind for the ProgramGroup.
 */
void ProgramGroup::set_kind(Kind kind)
{
    if(this->kind() != kind) {
        // We do something only if the program kind changes.
        this->description() = empty_description(kind, this->flags());
        // If the kind changes, functions_ are unrelated to the new kind
        functions_.clear();
    }
}

/**
 * Add a new Function (function name + Module in which it is defined) to the
 * ProgramGroup. Will trigger a build.
 *
 * @param kind     the type of the function. Might be:
 *                 - Function::Raygen
 *                 - Function::Miss
 *                 - Function::Exception
 *                 - Function::Intersection
 *                 - Function::AnyHit
 *                 - Function::ClosestHit
 *                 - Function::DirectCallable
 *                 - Function::ContinuationCallable
 * @param function a Function struct with the name and the Module in which the
 *                 function is defined.
 */
void ProgramGroup::add_function(const std::string& kind, const Function& function)
{
    if(!function.module) {
        throw std::runtime_error("Function module is null");
    }
    this->bump_version();
    functions_[kind] = function;
    // This allows to rebuild this object if the module changes.
    this->add_dependency(function.module);
}

/**
 * Return a function of a specific type. If the function is not found, a
 * FunctionNotFound exception is raised.
 *
 * @param kind     the type of the function. Might be:
 *                 - Function::Raygen
 *                 - Function::Miss
 *                 - Function::Exception
 *                 - Function::Intersection
 *                 - Function::AnyHit
 *                 - Function::ClosestHit
 *                 - Function::DirectCallable
 *                 - Function::ContinuationCallable
 * 
 * @return an iterator in functions_ container pointer to where the Function
 *         object is.
 */
ProgramGroup::Functions::const_iterator ProgramGroup::function(const std::string& kind) const
{
    auto it = functions_.find(kind);
    if(it == functions_.end()) {
        throw FunctionNotFound(kind);
    }
    return it;
}

/**
 * Sets a \_\_raygen\_\_ function and changes the Kind of the ProgramGroup to
 * OPTIX_PROGRAM_GROUP_KIND_RAYGEN.
 *
 * @param function a Function struct which name starts with \_\_raygen\_\_
 */
void ProgramGroup::set_raygen(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_RAYGEN);
    this->add_function(Function::Raygen, function);
}

/**
 * Sets a \_\_miss\_\_ function and changes the Kind of the ProgramGroup to
 * OPTIX_PROGRAM_GROUP_KIND_MISS.
 *
 * @param function a Function struct which name starts with \_\_miss\_\_
 */
void ProgramGroup::set_miss(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_MISS);
    this->add_function(Function::Miss, function);
}

/**
 * Sets an \_\_exception\_\_ function and changes the Kind of the ProgramGroup
 * to OPTIX_PROGRAM_GROUP_KIND_EXCEPTION
 *
 * @param function a Function struct which name starts with \_\_exception\_\_
 */
void ProgramGroup::set_exception(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_EXCEPTION);
    this->add_function(Function::Exception, function);
}

/**
 * Sets an \_\_intersection\_\_ function and changes the Kind of the
 * ProgramGroup to
 * [OPTIX_PROGRAM_GROUP_KIND_HITGROUP](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gabca35b1218b4df575a5c42926da0d978)
 *
 * @param function a Function struct which name starts with
 *                 \_\_intersection\_\_
 */
void ProgramGroup::set_intersection(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_HITGROUP);
    this->add_function(Function::Intersection, function);
}

/**
 * Sets an \_\_anythit\_\_ function and changes the Kind of the ProgramGroup to
 * OPTIX_PROGRAM_GROUP_KIND_HITGROUP.
 *
 * @param function a Function struct which name starts with \_\_anythit\_\_
 */
void ProgramGroup::set_anyhit(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_HITGROUP);
    this->add_function(Function::AnyHit, function);
}

/**
 * Sets an \_\_closestthit\_\_ function and changes the Kind of the
 * ProgramGroup to OPTIX_PROGRAM_GROUP_KIND_HITGROUP.
 *
 * @param function a Function struct which name starts with \_\_closestthit\_\_
 */
void ProgramGroup::set_closesthit(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_HITGROUP);
    this->add_function(Function::ClosestHit, function);
}

/**
 * Sets an \_\_direct_callable\_\_ function and changes the Kind of the
 * ProgramGroup to OPTIX_PROGRAM_GROUP_KIND_CALLABLES.
 *
 * @param function a Function struct which name starts with
 *                 \_\_direct_callable\_\_
 */
void ProgramGroup::set_direct_callable(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_CALLABLES);
    this->add_function(Function::DirectCallable, function);
}

/**
 * Sets an \_\_continuation_callable\_\_ function and changes the Kind of the
 * ProgramGroup to OPTIX_PROGRAM_GROUP_KIND_CALLABLES.
 *
 * @param function a Function struct which name starts with
 *                 \_\_continuation_callable\_\_
 */
void ProgramGroup::set_continuation_callable(const Function& function)
{
    this->set_kind(OPTIX_PROGRAM_GROUP_KIND_CALLABLES);
    this->add_function(Function::ContinuationCallable, function);
}

}; //namespace optix {
}; //namespace rtac { namespace optix {
