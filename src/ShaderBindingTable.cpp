#include <rtac_optix/ShaderBindingTable.h>

namespace rtac { namespace optix {

ShaderBindingTable::ShaderBindingTable(unsigned int raytypeCount) :
    raytypeCount_(raytypeCount),
    missRecords_(raytypeCount_, nullptr),
    objects_(0),
    hitRecordsCount_(0),
    hitRecordsSize_(0)
{}

ShaderBindingTable::Ptr ShaderBindingTable::Create(unsigned int raytypeCount)
{
    return Ptr(new ShaderBindingTable(raytypeCount));
}

void ShaderBindingTable::do_build() const
{
    this->fill_raygen_record();
    this->fill_exception_record();
    this->fill_miss_records();
    this->fill_hit_records();
}

void ShaderBindingTable::fill_raygen_record() const
{
    if(!raygenRecord_) {
        throw std::runtime_error(
            "No raygen record set. Cannot instanciate ShaderBindingTable");
    }
    std::vector<uint8_t> tmp(raygenRecord_->record_size());
    raygenRecord_->fill_sbt_record(tmp.data());
    raygenRecordData_ = tmp;
    optixObject_.raygenRecord = (CUdeviceptr)raygenRecordData_.data();
}

void ShaderBindingTable::fill_exception_record() const
{
    if(!exceptionRecord_)
        return;

    std::vector<uint8_t> tmp(exceptionRecord_->record_size());
    exceptionRecord_->fill_sbt_record(tmp.data());
    exceptionRecordData_ = tmp;
    optixObject_.exceptionRecord = (CUdeviceptr)exceptionRecordData_.data();
}

void ShaderBindingTable::fill_miss_records() const
{
    unsigned int recordSize = 0;
    for(auto mat : missRecords_) {
        if(mat)
            recordSize = std::max(recordSize, mat->record_size());
    }

    if(!recordSize)
        return;

    std::vector<uint8_t> tmp(raytypeCount_ * recordSize, 0);
    uint8_t* data = tmp.data();
    for(auto mat : missRecords_) {
        if(mat)
            mat->fill_sbt_record(data);
        data += recordSize;
    }
    missRecordsData_ = tmp;
    optixObject_.missRecordBase          = (CUdeviceptr)missRecordsData_.data();
    optixObject_.missRecordStrideInBytes = recordSize;
    optixObject_.missRecordCount         = raytypeCount_;
}

void ShaderBindingTable::fill_hit_records() const
{
    std::vector<uint8_t> recordsData(raytypeCount_ * hitRecordsCount_ * hitRecordsSize_, 0);
    for(auto& item : materials_) {
        for(auto offset : item.second) {
            item.first->fill_sbt_record(recordsData.data() + offset*hitRecordsSize_);
        }
    }
    hitRecordsData_ = recordsData;

    optixObject_.hitgroupRecordBase          = (CUdeviceptr)hitRecordsData_.data();
    optixObject_.hitgroupRecordStrideInBytes = hitRecordsSize_;
    optixObject_.hitgroupRecordCount         = raytypeCount_ * hitRecordsCount_;
}

unsigned int ShaderBindingTable::raytype_count() const
{
    return raytypeCount_;
}

const OptixShaderBindingTable* ShaderBindingTable::sbt() const
{
    this->build();
    return &optixObject_;
}

void ShaderBindingTable::set_raygen_record(
                    const ShaderBindingBase::ConstPtr& record)
{
    raygenRecord_ = record;
    this->add_dependency(record);
}

void ShaderBindingTable::set_exception_record(
                    const ShaderBindingBase::ConstPtr& record)
{
    exceptionRecord_ = record;
    this->add_dependency(record);
}

void ShaderBindingTable::add_miss_record(const MaterialBase::ConstPtr& record)
{
    if(record->raytype_index() >= raytypeCount_) {
        throw std::runtime_error("In valid miss record raytype index.");
    }
    missRecords_[record->raytype_index()] = record;
    this->add_dependency(record);
}

void ShaderBindingTable::add_material_record_index(
    const MaterialBase::ConstPtr& material, unsigned int index)
{
    if(materials_.find(material) == materials_.end()) {
        materials_[material] = std::vector<unsigned int>();
        this->add_dependency(material);
    }
    materials_[material].push_back(raytypeCount_*index + material->raytype_index());
    hitRecordsSize_ = std::max(hitRecordsSize_, material->record_size());
}

void ShaderBindingTable::add_object(const ObjectInstance::Ptr& object)
{
    if(std::find(objects_.begin(), objects_.end(), object) != objects_.end()) {
        // object already added.
        return;
    }

    objects_.push_back(object);
    objects_.back()->set_sbt_offset(hitRecordsCount_ * raytypeCount_);
    for(auto mat : objects_.back()->materials()) {
        if(mat.second)
            this->add_material_record_index(mat.second, hitRecordsCount_ + mat.first);
    }
    hitRecordsCount_ += object->material_count();
}

void ShaderBindingTable::set_raygen_program(const ProgramGroup::ConstPtr& program)
{
    this->set_raygen_record(ShaderBinding<void>::Create(program));
}

void ShaderBindingTable::set_exception_program(const ProgramGroup::ConstPtr& program)
{
    this->set_exception_record(ShaderBinding<void>::Create(program));
}


}; //namespace optix
}; //namespace rtac
