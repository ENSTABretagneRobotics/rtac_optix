#ifndef _DEF_RTAC_OPTIX_SHADER_BINDING_TABLE_H_
#define _DEF_RTAC_OPTIX_SHADER_BINDING_TABLE_H_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <rtac_base/cuda/DeviceVector.h>

#include <rtac_optix/OptixWrapper.h>
#include <rtac_optix/ShaderBinding.h>
#include <rtac_optix/Instance.h>
#include <rtac_optix/ObjectInstance.h>
#include <rtac_optix/GroupInstance.h>

namespace rtac { namespace optix {

class ShaderBindingTable : public OptixWrapper<OptixShaderBindingTable>
{
    public:

    using Ptr      = OptixWrapperHandle<ShaderBindingTable>;
    using ConstPtr = OptixWrapperHandle<const ShaderBindingTable>;

    using Buffer = cuda::DeviceVector<uint8_t>;

    protected:
    
    using MissRecords            = std::vector<MaterialBase::ConstPtr>;
    using MaterialRecordsIndexes = std::unordered_map<MaterialBase::ConstPtr,
                                                      std::vector<unsigned int>>;
                                                      //MaterialBase::ConstPtr::Hash>;
    
    // Attributes
    unsigned int raytypeCount_;

    ShaderBindingBase::ConstPtr raygenRecord_;
    mutable Buffer              raygenRecordData_;

    ShaderBindingBase::ConstPtr exceptionRecord_;
    mutable Buffer              exceptionRecordData_;

    MissRecords    missRecords_;
    mutable Buffer missRecordsData_;

    std::vector<ObjectInstance::Ptr> objects_; // Instances which contains materials
    MaterialRecordsIndexes           materials_;
    unsigned int                     hitRecordsCount_;
    unsigned int                     hitRecordsSize_;
    mutable Buffer                   hitRecordsData_;
    
    void add_material_record_index(const MaterialBase::ConstPtr& material, unsigned int index);

    void do_build() const;
    void fill_raygen_record() const;
    void fill_exception_record() const;
    void fill_miss_records() const;
    void fill_hit_records() const;

    ShaderBindingTable(unsigned int raytypeCount);

    public:

    static Ptr Create(unsigned int raytypeCount);
    
    unsigned int raytype_count() const;
    const OptixShaderBindingTable* sbt() const;
    
    void set_raygen_record(const ShaderBindingBase::ConstPtr& record);
    void set_exception_record(const ShaderBindingBase::ConstPtr& record);
    void add_miss_record(const MaterialBase::ConstPtr& record);
    void add_object(const ObjectInstance::Ptr& object);

    // BELOW HERE ARE ONLY HELPERS / OVERLOADS
    void set_raygen_program(const ProgramGroup::ConstPtr& program);
    void set_exception_program(const ProgramGroup::ConstPtr& program);
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_SHADER_BINDING_TABLE_H_
