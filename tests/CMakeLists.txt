# sources
#aux_source_directory(. SOURCES)
file(GLOB SOURCES test_*.c)

# flags
link_directories(${PROJECT_BINARY_DIR}/blosc)

# targets and tests
foreach(source ${SOURCES})
    get_filename_component(target ${source} NAME_WE)
    
    STRING(REGEX REPLACE "^.*(test_shuffle.c)$" "\\1" T_SHUFFLE ${source})
    STRING(COMPARE EQUAL "test_shuffle.c" "${T_SHUFFLE}" F_TSHUFFLE)
    if(NOT RES_SUPPORT_AVX2 AND F_TSHUFFLE)
        message(STATUS "Skip compile file test_shuffle.c ")
    else(NOT RES_SUPPORT_AVX2 AND F_TSHUFFLE)

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
    endif(NOT RES_SUPPORT_AVX2 AND F_TSHUFFLE)
endforeach(source)
