# sources
aux_source_directory(. SOURCES)


# flags
# @TODO:
# -O3 -Wall -fPIC -msse2
link_directories(${PROJECT_BINARY_DIR}/blosc)


# targets and tests
foreach(source ${SOURCES})
    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})
	target_link_libraries(${target} blosc_static)
    add_test(test_${target} ${target})
endforeach(source)
