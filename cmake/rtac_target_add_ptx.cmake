
# Saving this file location to find the generate_ptr_header.cmake file which is
# supposed to be in the same directory, either in the repository or installed
# on the system
set(RTAC_TARGET_ADD_PTX_LOCATION ${CMAKE_CURRENT_LIST_DIR})
function(target_add_ptx TARGET_NAME)

    # TAG_FOR_INSTALL             : Option to enable installation of the generated header.
    # OUTPUT_NAME <name>          : Name of the generated header.
    # INSTALL_DESTINATION <path>  : Install destination of the generated header relative to the installation path.
    #                               Will also be used as a prefix to generation path in the build directory.
    # CUDA_SOURCES <file_path...> : CUDA sources to be compiled and added to the generated header.
    # CUDA_OPTIONS <options...>   : Additional CUDA compile options for ptx files.

	cmake_parse_arguments(ARGUMENTS "TAG_FOR_INSTALL" "OUTPUT_NAME;INSTALL_DESTINATION" "CUDA_SOURCES;CUDA_OPTIONS" ${ARGN} )

    # output filename
    if("${ARGUMENTS_OUTPUT_NAME}" STREQUAL "")
        set(output_name "ptx_files.h")
    else()
        set(output_name "${ARGUMENTS_OUTPUT_NAME}")
    endif()

    if("${ARGUMENTS_INSTALL_DESTINATION}" STREQUAL "")
        set(ARGUMENTS_INSTALL_DESTINATION "${TARGET_NAME}")
    endif()

    # Creating an OBJECT target to generate the ptx files.
    set(ptx_target ${TARGET_NAME}_PTX_GEN)
    add_library(${ptx_target} OBJECT ${ARGUMENTS_CUDA_SOURCES})

    # set ptx target CUDA_ARCHITECTURES to the same value as TARGET_NAME's
    get_target_property(cuda_arch ${TARGET_NAME} CUDA_ARCHITECTURES)
    set_target_properties(${ptx_target} PROPERTIES
                          CUDA_ARCHITECTURES ${cuda_arch})

    # Enabling PTX generation
    set_target_properties(${ptx_target} PROPERTIES CUDA_PTX_COMPILATION ON)
    target_compile_options(${ptx_target} PUBLIC
        $<$<COMPILE_LANGUAGE:CUDA>: ${ARGUMENTS_CUDA_OPTIONS}>
    )

    # Set target properties of TARGET_NAME to ptx_target.
    get_target_property(target_include_dirs ${TARGET_NAME} INCLUDE_DIRECTORIES)
    if(NOT "${target_include_dirs}" STREQUAL "target_include_dirs-NOTFOUND")
        foreach(dir ${target_include_dirs})
            target_include_directories(${ptx_target} PRIVATE ${dir})
        endforeach()
    endif()
    get_target_property(target_link_libs ${TARGET_NAME} LINK_LIBRARIES)
    if(NOT "${target_link_libs}" STREQUAL "target_link_libs-NOTFOUND")
        foreach(lib ${target_link_libs})
            target_link_libraries(${ptx_target} PRIVATE ${lib})
        endforeach()
    endif()

    # After the generation of the PTX files but before the compilation of
    # TARGET_NAME, PTX files will be aggregated into a single header file. The
    # PTX code will be integrated directly into the compiled binaries, so
    # there is no runtime-dependency on installed headers.
    set(ptx_target_output_dir
        "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${ptx_target}.dir")

    # Generating the list of path to ptx files which will be generated 
    foreach(source ${ARGUMENTS_CUDA_SOURCES})
        
        get_filename_component(source_dir ${source} DIRECTORY)
        get_filename_component(name_wle   ${source} NAME_WLE)
        
        # Generating expected output ptx file name
        if(IS_ABSOLUTE ${source})
            set(ptx_output_file "${ptx_target_output_dir}${source_dir}/${name_wle}.ptx")
        else()
            set(ptx_output_file "${ptx_target_output_dir}/${source_dir}/${name_wle}.ptx")
        endif()
        list(APPEND ptx_files ${ptx_output_file})

    endforeach()
   
    # Command responsible to include all compilated PTX file into a C++ header
    # file. PTX strings can be retrieved from a filename keyed dictionary.
    set(full_output_path ${ptx_target_output_dir}/${ARGUMENTS_INSTALL_DESTINATION}/${output_name})
    add_custom_command(OUTPUT ${full_output_path}
                       DEPENDS ${ARGUMENTS_CUDA_SOURCES}
                       COMMAND ${CMAKE_COMMAND}
                       -DSOURCE_FILES="${ARGUMENTS_CUDA_SOURCES}"
                       -DPTX_FILES="${ptx_files}"
                       -DTARGET_NAME=${TARGET_NAME}
                       -DOUTPUT_FILE=${full_output_path}
                       -P ${RTAC_TARGET_ADD_PTX_LOCATION}/generate_ptx_header.cmake
                       COMMENT "Generating PTX header file ${ARGUMENTS_INSTALL_DESTINATION}/${output_name}")
    set(ptx_header_target ${TARGET_NAME}_PTX_GEN_HEADER)
    add_custom_target(${ptx_header_target} DEPENDS ${full_output_path})
    # This target must be built only when ptx_target is finished
    add_dependencies(${ptx_header_target} ${ptx_target})

    # Adding output directory to include directories for compilation
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${ptx_target_output_dir}>
    )
    # Adding a dependency to ensure header generation before compilation of
    # TARGET_NAME
    add_dependencies(${TARGET_NAME} ${ptx_header_target})

    if(ARGUMENTS_TAG_FOR_INSTALL)
        install(FILES ${full_output_path}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ARGUMENTS_INSTALL_DESTINATION})
    endif()

endfunction()






