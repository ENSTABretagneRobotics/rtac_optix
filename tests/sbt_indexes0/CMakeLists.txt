

set(target_name sbt_indexes0_${PROJECT_NAME})
add_executable(${target_name}
    src/sbt_indexes0.cpp
)
target_link_libraries(${target_name}
    rtac_optix
)
target_add_ptx(${target_name}
    CUDA_SOURCES src/sbt_indexes0.cu
)
