#include <rtac_optix/ObjectInstance.h>

namespace rtac { namespace optix {

ObjectInstance::ObjectInstance(const GeometryAccelStruct::ConstPtr& geometry,
                               unsigned int instanceId) :
    Instance(geometry, instanceId),
    materials_(0)
{}

/**
 * Creates a new ObjectInstance.
 *
 * @param geometry a (usually a MeshGeometry or CustomGeometry)
 */
ObjectInstance::Ptr ObjectInstance::Create(const GeometryAccelStruct::ConstPtr& geometry,
                                           unsigned int instanceId)
{
    return Ptr(new ObjectInstance(geometry, instanceId));
}

unsigned int ObjectInstance::material_count() const
{
    return this->child_->sbt_width();
}

void ObjectInstance::add_material(const MaterialBase::ConstPtr& material, unsigned int index)
{
    if(index >= this->material_count()) {
        throw std::runtime_error("Invalid index for material");
    }
    materials_.push_back(std::pair<unsigned int, MaterialBase::ConstPtr>(index, material));
    this->bump_version(false);
}

const ObjectInstance::Materials& ObjectInstance::materials() const
{
    return materials_;
}

}; //namespace optix
}; //namespace rtac
