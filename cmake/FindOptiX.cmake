# Written using guide from https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html 

# Helper function definitions (find process lower in this file)
function(optix_version_from_header OPTIX_INCLUDE_DIR)
    # Find optix.h from OPTIX_INCLUDE_DIR and parse OptiX version from it.
    # Return value : set OptiX_VERSION, OptiX_VERSION_MAJOR,
    # OptiX_VERSION_MINOR and OptiX_VERSION_PATCH in parent scope
    
    message(STATUS "Trying to guess OptiX version")
    
    # OptiX version is contained within OptiX main header optix.h
    find_file(OPTIX_H_PATH "optix.h" HINTS ${OPTIX_INCLUDE_DIR})
    if(NOT EXISTS ${OPTIX_H_PATH})
        message(FATAL_ERROR "Could not find optix.h in ${OPTIX_INCLUDE_DIR}")
    endif()
    
    # Iterating on optix.h lines to find the OptiX version
    file(STRINGS ${OPTIX_H_PATH} LINES)
    foreach(line ${LINES})
        # version number formated as "#define OPTIX_VERSION xx...x"
        
        # Identifying line on which version number is defined.
        string(REGEX MATCH
               "^\ *#define\ +OPTIX_VERSION\ +[0-9]+"
               VERSION_LINE ${line})
        if("${VERSION_LINE}" STREQUAL "")
            continue()
        endif()
        
        string(REGEX REPLACE
               "^\ *#define\ +OPTIX_VERSION\ +([0-9]+)*"
               "\\1"
               VERSION_STRING ${VERSION_LINE})
        if("${VERSION_STRING}" STREQUAL "")
            continue()
        endif()
        
        # Parsing version string according to specification inside optix.h
        math(EXPR VERSION_MAJOR "${VERSION_STRING} / 10000")
        math(EXPR VERSION_MINOR "(${VERSION_STRING} % 10000) / 100")
        math(EXPR VERSION_PATCH "${VERSION_STRING} % 100")

        set(OptiX_VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
        set(OptiX_VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
        set(OptiX_VERSION_PATCH ${VERSION_PATCH} PARENT_SCOPE)
        set(OptiX_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} PARENT_SCOPE)

        break()
    endforeach()
endfunction()

## Find process starts here
message(STATUS "Looking for OptiX")

set(OptiX_FOUND FALSE)

if(NOT "${OPTIX_PATH}" STREQUAL "")
    message(STATUS "Using user provided OPTIX_PATH : \"${OPTIX_PATH}\"")
elseif(NOT "$ENV{OPTIX_PATH}" STREQUAL "")
    set(OPTIX_PATH $ENV{OPTIX_PATH})
    message(STATUS "Using environment provided OPTIX_PATH : \"${OPTIX_PATH}\"")
else()
    message(STATUS "No OPTIX_PATH provided. Please provide the OptiX SDK location either by setting the environment variable \"OPTIX_PATH\" or by setting the cmake option -DOPTIX_PATH")
endif()

# Looking for Optix SDK (a bit dirty, find a better way)
find_path(OptiX_PREFIX "include/optix.h"
    HINTS ${OPTIX_PATH}
)
if( NOT EXISTS ${OptiX_PREFIX} )
    message(FATAL_ERROR "Cannot find OptiX SDK in ${OPTIX_PATH}")
endif()

# Now looking for optix.h header
find_path(OptiX_INCLUDE_DIRS "optix.h"
    HINTS ${OptiX_PREFIX}/include
)
if( NOT EXISTS ${OptiX_INCLUDE_DIRS} )
    message(FATAL_ERROR "Cannot find optix.h in ${OPTIX_PATH}")
endif()
message(STATUS "Found OptiX include location : ${OptiX_INCLUDE_DIRS}")

# optix_version_from_header will set the following variables :
# OptiX_VERSION
# OptiX_VERSION_MAJOR
# OptiX_VERSION_MINOR
# OptiX_VERSION_PATCH
optix_version_from_header(${OptiX_INCLUDE_DIRS})
message(STATUS "OptiX_VERSION : ${OptiX_VERSION}")
# message(STATUS "OptiX_VERSION_MAJOR : ${OptiX_VERSION_MAJOR}")
# message(STATUS "OptiX_VERSION_MINOR : ${OptiX_VERSION_MINOR}")
# message(STATUS "OptiX_VERSION_PATCH : ${OptiX_VERSION_PATCH}")

set(OptiX_FOUND TRUE)

# Looking for CUDA and nvrtc compiler interface
enable_language(CUDA)
find_package(CUDA REQUIRED)
find_library(CUDA_nvrtc_LIBRARY nvrtc
    PATHS ${CUDA_TOOLKIT_ROOT_DIR}
    PATH_SUFFIXES lib lib64
)

if(OptiX_FOUND AND NOT TARGET OptiX::OptiX)
    # add_library(OptiX::OptiX UNKNOWN IMPORTED)
    # Not IMPORTED because optix7 is all-headers. An all-header libray cannot
    # be an IMPORTED library because it is mandatory to provide an
    # IMPORTED_LOCATION and this would add a linker entry to link against,
    # which does not exists for an all-header library.
    # However, an INTERFACE library version handling seems non-existent.
    add_library(OptiX::OptiX INTERFACE IMPORTED) 
    target_link_libraries(OptiX::OptiX INTERFACE
        ${CUDA_LIBRARIES}
        ${CUDA_nvrtc_LIBRARY}
    )
    target_include_directories(OptiX::OptiX INTERFACE
        ${OptiX_INCLUDE_DIRS}
        ${CUDA_TOOLKIT_INCLUDE}
        ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    )
    set(NVRTC_INCLUDE_DIRS "${OptiX_INCLUDE_DIR}")
    set(NVRTC_INCLUDE_DIRS "${NVRTC_INCLUDE_DIRS}\\\;${OptiX_INCLUDE_DIR}/optixu")
    set(NVRTC_INCLUDE_DIRS "${NVRTC_INCLUDE_DIRS}\\\;${OptiX_PREFIX}/SDK/support/mdl-sdk/include")
    # set(NVRTC_INCLUDE_DIRS "${NVRTC_INCLUDE_DIRS}\\\;${OptiX_PREFIX}/SDK/sutils")
    set(NVRTC_INCLUDE_DIRS "${NVRTC_INCLUDE_DIRS}\\\;${OptiX_PREFIX}/SDK/cuda")
    set(NVRTC_INCLUDE_DIRS "${NVRTC_INCLUDE_DIRS}\\\;${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
    target_compile_definitions(OptiX::OptiX INTERFACE
        NVRTC_INCLUDE_DIRS="\\\"${NVRTC_INCLUDE_DIRS}\\\""
        NVRTC_COMPILE_OPTIONS="-use_fast_math -lineinfo -default-device -rdc=true -D__x86_64 -arch=compute_61"
    )
endif()

# including utility cmake modules
include(${CMAKE_CURRENT_LIST_DIR}/rtac_target_add_ptx.cmake)



