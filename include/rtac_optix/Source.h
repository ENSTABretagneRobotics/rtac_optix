#ifndef _DEF_RTAC_OPTIX_SOURCE_H_
#define _DEF_RTAC_OPTIX_SOURCE_H_

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

#include <rtac_optix/Handle.h>

namespace rtac { namespace optix {

class Source
{
    public:

    using Ptr      = Handle<Source>;
    using ConstPtr = Handle<const Source>;

    protected:

    std::string source_; // source code
    std::string name_;   // source name
                         // (in case of header, must be the #include <tag>)

    public:
    
    static Ptr New(const std::string& source, const std::string& name);
    Source(const std::string& source, const std::string& name);

    std::string source() const;
    std::string name()   const;

    const char* source_str() const;
    const char* name_str() const;

    int num_lines() const;
};

using Sources = std::vector<Source::ConstPtr>;

}; //namespace optix
}; //namespace rtac

rtac::optix::Sources operator+(const rtac::optix::Sources& lhs, 
                                 const rtac::optix::Sources& rhs);
std::ostream& operator<<(std::ostream& os, const rtac::optix::Source& source);
std::ostream& operator<<(std::ostream& os, const rtac::optix::Source::ConstPtr& source);
std::ostream& operator<<(std::ostream& os, const rtac::optix::Source::Ptr& source);

#endif //_DEF_RTAC_OPTIX_SOURCE_H_
