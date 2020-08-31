#include <optix_helpers/RayGenerator.h>

namespace optix_helpers {

RayGeneratorObj::Shape::Shape(size_t w, size_t h, size_t d) :
    width(w), height(h), depth(d)
{}

RayGeneratorObj::Shape::Shape(const std::initializer_list<size_t>& dims) :
    Shape()
{
    auto d = dims.begin();
    if(d == dims.end()) 
        return;
    width = *d;
    d++;
    if(d == dims.end()) 
        return;
    height = *d;
    d++;
    if(d == dims.end()) 
        return;
    depth = *d;
}

size_t RayGeneratorObj::Shape::size() const
{
    return width*height*depth;
}

RayGeneratorObj::RayGeneratorObj(size_t width, size_t height, size_t depth,
                                 const optix::Buffer& buffer,
                                 const Program& raygen,
                                 const Program& miss) :
    shape_(width, height, depth),
    renderBuffer_(buffer),
    raygenProgram_(raygen),
    missProgram_(miss)
{}

RayGenerator::RayGenerator() :
    Handle<RayGeneratorObj>()
{}

RayGenerator::RayGenerator(size_t width, size_t height, size_t depth,
                           const optix::Buffer& buffer,
                           const Program& raygen,
                           const Program& miss) :
    Handle<RayGeneratorObj>(new RayGeneratorObj(width, height, depth, buffer, raygen, miss))
{}

};
