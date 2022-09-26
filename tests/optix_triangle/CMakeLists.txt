

set(target_name optix_triangle_${PROJECT_NAME})
add_executable(${target_name}
    src/optix_triangle.cpp
)
target_link_libraries(${target_name} PRIVATE
    rtac_optix
)
target_add_ptx(${target_name}
    CUDA_SOURCES src/optix_triangle.cu
)


