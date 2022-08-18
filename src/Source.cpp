#include <rtac_optix/Source.h>

namespace rtac { namespace optix {

Source::Ptr Source::New(const std::string& source, const std::string& name)
{
    return Ptr(new Source(source, name));
}

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

const char* Source::source_str() const
{
    return source_.c_str();
}

const char* Source::name_str() const
{
    return name_.c_str();
}

int Source::num_lines() const
{
    return std::count(source_.begin(), source_.end(), '\n');
}

}; //namespace optix
}; //namespace rtac

rtac::optix::Sources operator+(const rtac::optix::Sources& lhs, 
                                 const rtac::optix::Sources& rhs)
{
    rtac::optix::Sources res = lhs;
    res.insert(res.end(), rhs.begin(), rhs.end());
    return res;
}

std::ostream& operator<<(std::ostream& os, const rtac::optix::Source& source)
{
    os << "Optix source file " << source.name() << "\n";
    int Nlines = source.num_lines();
    int padWidth = std::to_string(Nlines - 1).size();
    std::istringstream iss(source.source());
    int lineIdx = 1;
    for(std::string line; std::getline(iss, line); lineIdx++) {
        os << std::setw(padWidth) << lineIdx << ' ' << line << '\n';
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const rtac::optix::Source::ConstPtr& source)
{
    os << *source;
    return os;
}

std::ostream& operator<<(std::ostream& os, const rtac::optix::Source::Ptr& source)
{
    os << *source;
    return os;
}

