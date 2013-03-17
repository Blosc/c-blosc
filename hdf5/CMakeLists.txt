# sources
set(SOURCES blosc_filter.c)


# dependencies
find_package(HDF5 REQUIRED)
include_directories(HDF5_INCLIDE_DIRS)


# flags
# @TODO:
# -O3 -Wall -fPIC -msse2


# targets
add_library(blosc_filter_static ${SOURCES})
set_target_properties(blosc_filter_static PROPERTIES OUTPUT_NAME blosc_filter)
target_link_libraries(blosc_filter_static blosc_static)

if(BUILD_SHARED)
    add_library(blosc_filter_shared SHARED ${SOURCES})
	set_target_properties(blosc_filter_shared PROPERTIES OUTPUT_NAME blosc_filter)
	target_link_libraries(blosc_filter_shared blosc_shared ${HDF5_LIBRARIES})
endif(BUILD_SHARED)


# install
install(FILES blosc_filter.h DESTINATION include)
install(TARGETS blosc_filter_static DESTINATION lib COMPONENT HDF5_FILTER)
if(BUILD_SHARED)
    install(TARGETS blosc_filter_shared DESTINATION lib COMPONENT HDF5_FILTER)
endif(BUILD_SHARED)


# test
if(BUILD_TESTS)
	add_executable(example example.c)
	target_link_libraries(example blosc_filter_static ${HDF5_LIBRARIES})
	add_test(test_hdf5_filter example)
endif(BUILD_TESTS)
