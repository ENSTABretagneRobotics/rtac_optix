

set(target_name mesh_test_${PROJECT_NAME})
add_executable(${target_name}
    src/mesh_test.cpp
)
target_link_libraries(${target_name} PRIVATE
    rtac_optix
)
set_target_properties(${target_name} PROPERTIES CUDA_ARCHITECTURES native)
target_add_ptx(${target_name}
    CUDA_SOURCES src/mesh_test.cu
)
