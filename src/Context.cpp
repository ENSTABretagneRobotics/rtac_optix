#include <optix_helpers/Context.h>

namespace optix_helpers {

ContextObj::ContextObj() :
    context_(optix::Context::create())
{
}


optix::Context ContextObj::context() const
{
    return context_;
}

unsigned int ContextObj::num_raytypes() const
{
    return this->context()->getRayTypeCount();
}

Program ContextObj::create_program(const Source& source, const Sources& additionalHeaders) const
{
    try {
        auto ptx = nvrtc_.compile(source, additionalHeaders);
        optix::Program program = context_->createProgramFromPTXString(ptx, source->name());
        return Program(source, additionalHeaders, program);
    }
    catch(const std::runtime_error& e) {
        std::ostringstream os;
        os << source <<  "\n" << e.what();
        throw std::runtime_error(os.str());
    }
}

RayGenerationProgram ContextObj::create_raygen_program(const std::string renderBufferName,
                                                       const Source& source,
                                                       const Sources& additionalHeaders) const
{
    return RayGenerationProgram(this->create_program(source, additionalHeaders),
                                renderBufferName);
}

RayType ContextObj::create_raytype(const Source& rayDefinition) const
{
    unsigned int rayTypeIndex = this->num_raytypes();
    this->context()->setRayTypeCount(rayTypeIndex + 1);
    return RayType(rayTypeIndex, rayDefinition);
}

Material ContextObj::create_material() const
{
    return Material(this->context()->createMaterial());
}

Geometry ContextObj::create_geometry(const Program& intersection,
                                     const Program& boundingbox,
                                     size_t primitiveCount) const
{
    return Geometry(context_->createGeometry(), intersection, boundingbox, primitiveCount);
}

GeometryTriangles ContextObj::create_geometry_triangles() const
{
    return GeometryTriangles(context_->createGeometryTriangles(),
                             context_->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3),
                             context_->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT3));
}

Model ContextObj::create_model() const
{
    return Model(context_->createGeometryInstance());
}

RayGenerator ContextObj::create_raygenerator(size_t width, size_t height, size_t depth) const
{
    return RayGenerator(width, height, depth, context_->createBuffer(RT_BUFFER_OUTPUT));
}

SceneItem ContextObj::create_scene_item(const Model& model) const
{
    return SceneItem(context_->createGeometryGroup(),
                     context_->createTransform(),
                     model);
}

optix::Handle<optix::VariableObj> ContextObj::operator[](const std::string& varname)
{
    return context_[varname];
}

Context::Context() :
    Handle<ContextObj>(new ContextObj)
{}

optix::Handle<optix::VariableObj> Context::operator[](const std::string& varname)
{
    return (*this)[varname];
}

} //namespace optix_helpers

