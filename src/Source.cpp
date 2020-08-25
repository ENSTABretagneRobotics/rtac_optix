#include <optix_helpers/Source.h>

namespace optix_helpers {

Source::Source(const std::string& source, const std::string& name) :
    source_(source),
    name_(name)
{
}

std::string Source::source() const
{
    return source_;
}

std::string Source::name() const
{
    return name_;
}

}; //namespace optix_helpers
