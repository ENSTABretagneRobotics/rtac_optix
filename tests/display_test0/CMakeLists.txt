

set(target_name display_test0_${PROJECT_NAME})
if(NOT TARGET rtac_display)
    find_package(rtac_display REQUIRED)
endif()
add_executable(${target_name}
    src/display_test0.cpp
)
target_link_libraries(${target_name}
    rtac_optix
    rtac_display
)
target_add_ptx(${target_name}
    CUDA_SOURCES src/display_test0.cu
)
