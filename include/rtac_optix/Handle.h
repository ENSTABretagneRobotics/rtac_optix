#ifndef _DEF_RTAC_OPTIX_HANDLE_H_
#define _DEF_RTAC_OPTIX_HANDLE_H_

#include <rtac_base/types/Handle.h>

namespace rtac { namespace optix {

template <typename T>
using Handle = rtac::types::Handle<T>; // Is actually a std::shared_ptr

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_HANDLE_H_
