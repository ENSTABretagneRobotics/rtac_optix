#include <optix_helpers/Source.h>

namespace optix_helpers {

SourceObj::SourceObj(const std::string& source, const std::string& name) :
    source_(source),
    name_(name)
{
}

SourceObj::SourceObj(const SourceObj& other) :
    source_(other.source()),
    name_(other.name())
{
}

std::string SourceObj::source() const
{
    return source_;
}

std::string SourceObj::name() const
{
    return name_;
}

int SourceObj::num_lines() const
{
    return std::count(source_.begin(), source_.end(), '\n');
}

std::ostream& SourceObj::print(std::ostream& os) const
{
    int Nlines = this->num_lines();
    int padWidth = std::to_string(Nlines - 1).size();
    std::istringstream iss(source_);
    int lineIdx = 1;
    for(std::string line; std::getline(iss, line); lineIdx++) {
        os << std::setw(padWidth) << lineIdx << ' ' << line << '\n';
    }
    return os;
}

// Source type implementation
Source::Source() :
    Handle<SourceObj>()
{}

Source::Source(const std::string& source, const std::string& name) :
    Handle<SourceObj>(new SourceObj(source, name))
{}

}; //namespace optix_helpers


std::ostream& operator<<(std::ostream& os, const optix_helpers::Source& source)
{
    if(!source) {
        os << "Empty source file.";
        return os;
    }
    os << "Optix source file " << source->name() << "\n";
    source->print(os);
    return os;
}
