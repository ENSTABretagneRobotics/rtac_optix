

set(target_name sbt_test0_${PROJECT_NAME})
add_executable(${target_name}
    src/sbt_test0.cpp
)
target_link_libraries(${target_name}
    rtac_optix
)
target_add_ptx(${target_name}
    CUDA_SOURCES src/sbt_test0.cu
)
