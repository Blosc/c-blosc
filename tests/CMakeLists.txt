# sources
#aux_source_directory(. SOURCES)
file(GLOB SOURCES test_*.c)

# flags
link_directories(${PROJECT_BINARY_DIR}/blosc)

# targets and tests
foreach(source ${SOURCES})
    # Enable support for testing accelerated shuffles
    if(COMPILER_SUPPORT_SSE2)
        # Define a symbol so tests for SSE2 shuffle/unshuffle will be compiled in.
        set_property(
            SOURCE ${source}
            APPEND PROPERTY COMPILE_DEFINITIONS SHUFFLE_SSE2_ENABLED)
    endif(COMPILER_SUPPORT_SSE2)
#    if(COMPILER_SUPPORT_AVX2)
#        # Define a symbol so tests for AVX2 shuffle/unshuffle will be compiled in.
#        set_property(
#            SOURCE ${source}
#            APPEND PROPERTY COMPILE_DEFINITIONS SHUFFLE_AVX2_ENABLED)
#    endif(COMPILER_SUPPORT_AVX2)

    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})

    # have to copy dlls for Visual Studio
    if(MSVC)
        add_custom_command(
            TARGET      ${target}
            POST_BUILD
            COMMAND     ${CMAKE_COMMAND}
            ARGS        -E copy_if_different
                        "${PROJECT_BINARY_DIR}/blosc/\$\(Configuration\)/blosc.dll"
                        "${CMAKE_CURRENT_BINARY_DIR}/\$\(Configuration\)/blosc.dll")   
    endif(MSVC)

    target_link_libraries(${target} blosc_shared)
    add_test(test_${target} ${target})
endforeach(source)
