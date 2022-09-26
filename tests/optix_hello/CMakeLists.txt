

set(target_name optix_hello_${PROJECT_NAME})
add_executable(${target_name}
    src/optix_hello.cpp
)
target_link_libraries(${target_name} PRIVATE rtac_optix)
target_add_ptx(${target_name}
    CUDA_SOURCES src/draw_solid_color.cu
)

