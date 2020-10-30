#include <optix_helpers/Buffer.h>

namespace optix_helpers {

Buffer::Ptr Buffer::New(const Context::ConstPtr& context, 
                        RTbuffertype bufferType,
                        RTformat format,
                        const std::string& name)
{
    return Ptr(new Buffer(context, bufferType, format, name));
}


Buffer::Buffer(const Context::ConstPtr& context, 
               RTbuffertype bufferType,
               RTformat format,
               const std::string& name) :
    NamedObject<optix::Buffer>(context->context()->createBuffer(bufferType, format), name)
{}

Buffer::Buffer(const optix::Buffer& buffer, const std::string& name) :
    NamedObject<optix::Buffer>(buffer, name)
{}

const optix::Buffer Buffer::buffer() const
{
    return object_;
}

void Buffer::set_size(size_t width, size_t height)
{
    object_->setSize(width, height);
}

optix::Buffer Buffer::buffer()
{
    return object_;
}

Buffer::Shape Buffer::shape() const
{
    Shape res;
    this->buffer()->getSize(res.width, res.height);
    return res;
}

size_t Buffer::size() const
{
    return this->shape().area();
}

void Buffer::unmap() const
{
    object_->unmap();
}

}; //namespace optix_helpers

