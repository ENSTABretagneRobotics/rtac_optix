#ifndef _DEF_RTAC_OPTIX_NVRTC_H_
#define _DEF_RTAC_OPTIX_NVRTC_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <nvrtc.h>

#include <rtac_optix/Source.h>

namespace rtac { namespace optix {

class Nvrtc
{
    public :

    using StringList = std::list<std::string>;

    protected:
    
    // These are for convenience
    StringList includeDirs_;
    StringList compileOptions_;

    std::vector<const char*> nvrtc_options() const;
    nvrtcProgram program_;
    std::string compilationLog_;

    public:
    
    Nvrtc();
    Nvrtc(const Nvrtc& other);
    ~Nvrtc();
    void load_default_include_dirs();
    void load_default_compile_options();
    
    void clear_include_dirs();
    void clear_compile_options();
    void clear_program();
    void clear_all();

    void add_include_dirs(const std::string& dirs);
    void add_include_dirs(const StringList& dirs);
    void add_compile_options(const std::string& options);
    void add_compile_options(const StringList& options);
    
    std::string compile(const Source::ConstPtr& source,
                        const Sources& additionalHeaders = Sources());
    void update_log();

    // getters
    StringList include_dirs()    const;
    StringList compile_options() const;
    std::string get_ptx()        const;
    nvrtcProgram release_program(); //This won't destroy the program. This is to
                                    //give the ownership to another scope.

    // static functions
    static StringList parse_option_string(const std::string& options,
                                          const std::string& separator);
    static std::string load_source_file(const std::string& path);
    static void check_error(nvrtcResult errorCode);
};

}; //namespace optix
}; //namespace rtac

std::ostream& operator<<(std::ostream& os, const rtac::optix::Nvrtc& nvrtc);

#endif //_DEF_RTAC_OPTIX_NVRTC_H_


