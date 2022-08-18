#include <rtac_optix/Nvrtc.h>

#include <cstring>

#ifndef NVRTC_INCLUDE_DIRS
#define NVRTC_INCLUDE_DIRS ""
#endif
#ifndef NVRTC_COMPILE_OPTIONS
#define NVRTC_COMPILE_OPTIONS "-use_fast_math -lineinfo -default-device -rdc=true -D__x86_64"
#endif

namespace rtac { namespace optix {

Nvrtc::Nvrtc() :
    program_(0)
{
    this->load_default_include_dirs();
    this->load_default_compile_options();
}

Nvrtc::Nvrtc(const Nvrtc& other) :
    includeDirs_(other.includeDirs_),
    compileOptions_(other.compileOptions_),
    program_(0)
{
}

Nvrtc::~Nvrtc()
{
}

void Nvrtc::load_default_include_dirs()
{
    // NVRTC_INCLUDE_DIRS was defined at optix installation and contains mostly
    // some paths to th optix SDK.
    this->add_include_dirs(parse_option_string(NVRTC_INCLUDE_DIRS, ";"));
}

void Nvrtc::load_default_compile_options()
{
    this->add_compile_options(NVRTC_COMPILE_OPTIONS);
}

void Nvrtc::clear_include_dirs()
{
    includeDirs_.clear();
}

void Nvrtc::clear_compile_options()
{
    compileOptions_.clear();
}

void Nvrtc::clear_program()
{
    if(program_ != 0) {
        check_error(nvrtcDestroyProgram(&program_));
        program_ = 0;
    }
}

void Nvrtc::clear_all()
{
    this->clear_include_dirs();
    this->clear_compile_options();
    this->clear_program();
}

void Nvrtc::add_include_dirs(const std::string& dirs)
{
    this->add_include_dirs(parse_option_string(dirs, " "));
}

void Nvrtc::add_include_dirs(const StringList& dirs)
{
    for(auto dir : dirs) {
        compileOptions_.push_back("-I" + dir);
    }
}

void Nvrtc::add_compile_options(const std::string& options)
{
    this->add_compile_options(parse_option_string(options, " "));
}

void Nvrtc::add_compile_options(const StringList& options)
{
    for(auto opt : options) {
        compileOptions_.push_back(opt);
    }
}

std::string Nvrtc::compile(const Source::ConstPtr& source, const Sources& additionalHeaders)
{
    std::vector<const char*> options = this->nvrtc_options();
    std::vector<const char*> headers(additionalHeaders.size());
    std::vector<const char*> hnames(additionalHeaders.size());

    auto header = additionalHeaders.cbegin();
    for(int i = 0; i < headers.size(); i++) {
        headers[i] = (*header)->source_str();
        hnames[i]  = (*header)->name_str();
        header++;
    }
    
    this->clear_program();
    check_error(nvrtcCreateProgram(&program_, source->source_str(), source->name_str(),
                                   headers.size(), headers.data(), hnames.data()));
    try {
        check_error(nvrtcCompileProgram(program_, options.size(), options.data()));
    }
    catch(const std::runtime_error& e) {
        this->update_log();
        throw std::runtime_error("NVRTC compilation failed :\n" + compilationLog_);
    }
    this->update_log();

    return this->get_ptx();
}

void Nvrtc::update_log()
{
    if(program_ == 0)
        return;
    size_t logSize = 0;
    check_error(nvrtcGetProgramLogSize(program_, &logSize));
    if(logSize > 1) {
        compilationLog_.resize(logSize);
        check_error(nvrtcGetProgramLog(program_, &compilationLog_[0]));
    }
}

Nvrtc::StringList Nvrtc::include_dirs() const
{
    return includeDirs_;
}

Nvrtc::StringList Nvrtc::compile_options() const
{
    return compileOptions_;
}

std::string Nvrtc::get_ptx() const
{
    std::string ptx("");

    if(program_ == 0)
        return ptx;

    size_t ptxSize;
    check_error(nvrtcGetPTXSize(program_, &ptxSize));
    ptx.resize(ptxSize);
    check_error(nvrtcGetPTX(program_, &ptx[0]));

    return ptx;
}

nvrtcProgram Nvrtc::release_program()
{
    nvrtcProgram program = program_;
    program_ = 0;
    return program;
}

std::vector<const char*> Nvrtc::nvrtc_options() const
{
    // build an array of const char* because that's what expects NVRTC as input
    std::vector<const char*> options(includeDirs_.size() + compileOptions_.size());
    int idx = 0;
    for(auto& dir : includeDirs_) {
        options[idx] = dir.c_str();
        idx++;
    }
    for(auto& opt : compileOptions_) {
        options[idx] = opt.c_str();
        idx++;
    }
    return options;
}

// static methods
Nvrtc::StringList Nvrtc::parse_option_string(const std::string& options,
                                                           const std::string& separator)
{
    StringList parsedOptions;

    // removing unwanted quotes (results from setting the definition through
    // cmake. See if better way)
    std::string opts(options);
    if(opts[0] == '\"')
        opts.erase(0,1);
    if(opts[opts.size()-1] == '\"')
        opts.pop_back();

    parsedOptions.clear();
    size_t pos;
    while((pos = opts.find(separator)) != std::string::npos) {
        parsedOptions.push_back(opts.substr(0, pos));
        opts.erase(0, pos + separator.size());
    }
    if(opts.size() > 0)
        parsedOptions.push_back(opts);

    return parsedOptions;
}

std::string Nvrtc::load_source_file(const std::string& path)
{
    std::ifstream f;
    f.open(path, std::ios::in);
    if(!f)
        throw std::runtime_error("Cloud not open " + path);
    std::ostringstream contents;
    contents << f.rdbuf();
    f.close();

    return contents.str();
}

void Nvrtc::check_error(nvrtcResult errorCode)
{
    if(errorCode != NVRTC_SUCCESS) {
        throw std::runtime_error("NVRTC compilation error : " +
                                 std::string(nvrtcGetErrorString(errorCode)));
    }
}

}; //namespace optix
}; //namespace rtac

std::ostream& operator<<(std::ostream& os, const rtac::optix::Nvrtc& nvrtc)
{
    os << "NVRTC compile options :\n";
    os << "Include dirs :";
    for(auto dir : nvrtc.include_dirs()) {
        os << "\n " << dir;
    }
    os << "\nCompile options :";
    for(auto dir : nvrtc.compile_options()) {
        os << "\n " << dir;
    }
    return os;
}

