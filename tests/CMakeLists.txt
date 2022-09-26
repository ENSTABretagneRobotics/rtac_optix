add_executable(compile_test
    src/compile_test.cpp
    src/compile_test.cu
)
target_link_libraries(compile_test rtac_optix)
target_add_ptx(compile_test
    CUDA_SOURCES src/compile_test.cu
)

list(APPEND test_names
    src/nvrtc_ptx.cpp
)
foreach(filename ${test_names})
    get_filename_component(test_name ${filename} NAME_WE)
    set(target_name ${test_name}_${PROJECT_NAME})

    add_executable(${target_name} ${filename})
    target_link_libraries(${target_name} rtac_optix)
endforeach()

add_subdirectory(optix_hello)
add_subdirectory(optix_triangle)
add_subdirectory(pinhole)
add_subdirectory(mesh)
add_subdirectory(sbt_test0)
add_subdirectory(display_test0)
add_subdirectory(sbt_indexes0)
add_subdirectory(materials_test)
add_subdirectory(autosbt_test)
add_subdirectory(helpers)




