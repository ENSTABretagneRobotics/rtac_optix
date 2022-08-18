#ifndef _DEF_RTAC_PROGRAM_GROUP_H_
#define _DEF_RTAC_PROGRAM_GROUP_H_

#include <iostream>
#include <sstream>
#include <unordered_map>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/utils.h>
#include <rtac_optix/Handle.h>
#include <rtac_optix/Context.h>
#include <rtac_optix/Module.h>
#include <rtac_optix/OptixWrapper.h>

namespace rtac { namespace optix {

class Pipeline;

/**
 * A wrapper around the OptixProgramGroup type.
 *
 * A ProgramGroup represents a single or a set of user defined functions to be
 * called by OptiX on GPU during ray-tracing.
 *
 * The functions defined in the ProgramGroup only operates on a single ray at a
 * time. But several thousands of these functions run concurrently on the GPU
 * to achieve parallel ray-tracing.
 *
 * There are 5 kind of ProgramGroup, each called on a specific ray-tracing step :
 * - OPTIX_PROGRAM_GROUP_KIND_RAYGEN : the \_\_raygen\_\_ function can be
 *      considered as the "main function" for rays.  It is the first function
 *      to be called on each independent ray on a ray-tacing launch and the
 *      last one to returns at the end of the ray-tracing operation. It is
 *      responsible to launch the first of potentially many ray recursions and
 *      usually fills the result of the ray-tracing in the ouput buffer. There
 *      are as many \_\_raygen\_\_ calls than the concurrent rays computations
 *      on the GPU.
 * -
 * OPTIX_PROGRAM_GROUP_KIND_MISS : the \_\_miss\_\_ function is called when a
 *      ray reached its maximum distance without intersecting anything. 
 * -
 * OPTIX_PROGRAM_GROUP_KIND_HITGROUP : a hitgroup ProgramGroup contains several
 *      functions all of which are optional. The \_\_anyhit\_\_ function is
 *      called on any intersection of the ray with a primitive. They are as
 *      much \_\_anyhit\_\_ calls as there are intersections. The
 *      \_\_closesthit\_\_ function is called only on the intersection the
 *      closest to the origin of the ray. The \_\_intersection\_\_ function is
 *      called when a ray intersects the bounding-box of a primitive.  The
 *      \_\_intersection\_\_ function reports to the OptiX API if an
 *      intersection with the primitive occurred within the bounding box. If
 *      the \_\_intersection\_\_ function does not report an intersection to
 *      OptiX, the \_\_anyhit\_\_ and \_\_closesthit\_\_ function won't be
 *      called on this primitive. The \_\_intersection\_\_ function is used to
 *      define custom primitive geometries (i.e. non-triangles) such as
 *      spheres, or any other geometry. 
 * - OPTIX_PROGRAM_GROUP_KIND_EXCEPTION : Not tested in rtac_optix yet.
 * - OPTIX_PROGRAM_GROUP_KIND_CALLABLES : Not tested in rtac_optix yet.
 */
class ProgramGroup : public OptixWrapper<OptixProgramGroup>
{
    public:

    friend class Pipeline;
    
    using Ptr      = OptixWrapperHandle<ProgramGroup>;
    using ConstPtr = OptixWrapperHandle<const ProgramGroup>;
    
    /**
     * Simple pair of a function name and a Module to simplify the ProgramGroup
     * API.
     */
    struct Function {

        static const char* Raygen;
        static const char* Miss;
        static const char* Exception;
        static const char* Intersection;
        static const char* AnyHit;
        static const char* ClosestHit;
        static const char* DirectCallable;
        static const char* ContinuationCallable;

        std::string name;
        Module::ConstPtr module;
    };
    struct FunctionNotFound : public std::runtime_error {
        FunctionNotFound(const std::string& kind) : 
            std::runtime_error(
                std::string("ProgramGroup has no \"") + kind + "\" function.")
        {}
    };

    using Kind        = OptixProgramGroupKind;
    using Description = OptixProgramGroupDesc;
    using Options     = OptixProgramGroupOptions;
    using Functions   = std::unordered_map<std::string, Function>;
    
    static Description empty_description(const Kind& kind,
        unsigned int flags = OPTIX_PROGRAM_GROUP_FLAGS_NONE);

    // This is not used by optix-7. Leaving this here for future compatibility.
    static Options default_options();

    protected:
    
    Context::ConstPtr   context_;
    mutable Description description_;
    Options             options_;
    Functions           functions_;
    
    // Making these protected to force the user to use the set_* methods below
    Description& description();
    void add_function(const std::string& kind, const Function& function);

    void update_description() const;
    virtual void do_build() const;
    virtual void clean() const;

    ProgramGroup(const Context::ConstPtr& context, Kind kind,
                 unsigned int flags = OPTIX_PROGRAM_GROUP_FLAGS_NONE,
                 const Options& options = default_options());
    
    static Ptr Create(const Context::ConstPtr& context, Kind kind,
                      unsigned int flags = OPTIX_PROGRAM_GROUP_FLAGS_NONE,
                      const Options& options = default_options());
    public:

    ~ProgramGroup();

    const Description& description() const;
    const Options& options() const;
    Options& options();

    Kind kind() const;
    unsigned int flags() const;
    void set_kind(Kind kind);

    Functions::const_iterator function(const std::string& kind) const;

    void set_raygen(const Function& function);
    void set_miss(const Function& function);
    void set_exception(const Function& function);
    void set_intersection(const Function& function);
    void set_anyhit(const Function& function);
    void set_closesthit(const Function& function);
    void set_direct_callable(const Function& function);
    void set_continuation_callable(const Function& function);
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_PROGRAM_GROUP_H_
