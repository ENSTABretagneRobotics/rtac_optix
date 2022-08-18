cmake_minimum_required(VERSION 3.10)
project(rtac_optix VERSION 0.1 LANGUAGES CUDA CXX)

option(BUILD_TESTS "Build unit tests." ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(OptiX REQUIRED)

if(NOT TARGET rtac_base)
    find_package(rtac_base REQUIRED)
endif()
if(NOT TARGET rtac_cuda)
    find_package(rtac_cuda REQUIRED)
endif()

list(APPEND rtac_optix_PUBLIC_HEADERS
    include/rtac_optix/Handle.h
    include/rtac_optix/Source.h
    include/rtac_optix/Nvrtc.h
    include/rtac_optix/utils.h
    include/rtac_optix/OptixWrapper.h
    include/rtac_optix/Context.h
    include/rtac_optix/Module.h
    include/rtac_optix/ProgramGroup.h
    include/rtac_optix/Pipeline.h

    include/rtac_optix/TraversableHandle.h
    include/rtac_optix/Instance.h
    include/rtac_optix/AccelerationStruct.h
    include/rtac_optix/GeometryAccelStruct.h
    include/rtac_optix/MeshGeometry.h
    include/rtac_optix/CustomGeometry.h
    include/rtac_optix/InstanceAccelStruct.h
    

    include/rtac_optix/Raytype.h
    include/rtac_optix/RaytypeFactory.h
    include/rtac_optix/ShaderBinding.h
    include/rtac_optix/ShaderBindingTable.h
    include/rtac_optix/Material.h
    include/rtac_optix/ObjectInstance.h
    include/rtac_optix/GroupInstance.h

    # These files were copied from the optix-7.2 SDK
    include/rtac_optix/optix_SDK/sutil/Preprocessor.h
    include/rtac_optix/optix_SDK/sutil/vec_math.h
    
    include/rtac_optix/helpers/maths.h
    include/rtac_optix/helpers/PinholeCamera.h
)
list(APPEND rtac_optix_SOURCES
    src/Source.cpp
    src/Nvrtc.cpp
    src/Context.cpp
    src/Module.cpp
    src/ProgramGroup.cpp
    src/Pipeline.cpp

    src/Instance.cpp
    src/AccelerationStruct.cpp
    src/GeometryAccelStruct.cpp
    src/MeshGeometry.cpp
    src/CustomGeometry.cpp
    src/InstanceAccelStruct.cpp

    src/ShaderBinding.cpp
    src/ShaderBindingTable.cpp
    src/Material.cpp
    src/ObjectInstance.cpp
    src/GroupInstance.cpp
)

add_library(rtac_optix SHARED ${rtac_optix_SOURCES})
set_target_properties(rtac_optix PROPERTIES 
    RTAC_PUBLIC_HEADERS "${rtac_optix_PUBLIC_HEADERS}"
    INSTALL_RPATH_USE_LINK_PATH True
    CUDA_ARCHITECTURES native
)
target_include_directories(rtac_optix
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/rtac_optix/optix_SDK>
        $<INSTALL_INTERFACE:include/rtac_optix/optix_SDK>
)
target_link_libraries(rtac_optix PUBLIC
    OptiX::OptiX
    rtac_base
    rtac_cuda
)
rtac_install_target(rtac_optix
    ADDITIONAL_CONFIG_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindOptiX.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/rtac_target_add_ptx.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_ptx_header.cmake
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()


