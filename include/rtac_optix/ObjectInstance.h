#ifndef _DEF_RTAC_OPTIX_OBJECT_INSTANCE_H_
#define _DEF_RTAC_OPTIX_OBJECT_INSTANCE_H_

#include <iostream>
#include <vector>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/Handle.h>
#include <rtac_optix/utils.h>
#include <rtac_optix/GeometryAccelStruct.h>
#include <rtac_optix/Instance.h>
#include <rtac_optix/Material.h>

namespace rtac { namespace optix {

/**
 * Subclass of Instance representing a single object in the object tree (not a
 * group of objects). This is the recommended way to create an object.
 *
 * This class binds together a single geometry which gives the object its
 * shape, and one or several Materials which define the behavior of the rays
 * which intersect with the geometry.
 */
class ObjectInstance : public Instance
{
    public:

    using Ptr       = OptixWrapperHandle<ObjectInstance>;
    using ConstPtr  = OptixWrapperHandle<const ObjectInstance>;
    using Materials = std::vector<std::pair<unsigned int, MaterialBase::ConstPtr>>;
    
    protected:

    Materials materials_;

    ObjectInstance(const GeometryAccelStruct::ConstPtr& geometry,
                   unsigned int instanceId = 0);

    public:

    static Ptr Create(const GeometryAccelStruct::ConstPtr& geometry,
                      unsigned int instanceId = 0);
    
    unsigned int material_count() const;
    void add_material(const MaterialBase::ConstPtr& material,
                      unsigned int index = 0);
    const Materials& materials() const;
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_OBJECT_INSTANCE_H_
