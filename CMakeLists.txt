cmake_minimum_required(VERSION 2.6)
project(blosc)


# version numbers
# @TODO: set version numbers by parsing the bloasc.h header
set(BLOSC_VERSION_MAJOR 1)
set(BLOSC_VERSION_MINOR 1)
set(BLOSC_VERSION_PATCH 6)  # BLOSC_VERSION_RELEASE


# options
option(BUILD_STATIC
    "Build a static version of the blosc library." ON)
option(BUILD_HDF5_FILTER
    "Build a blosc based compression filter for the HDF5 library" ON)
option(BUILD_TESTS
    "Build test programs form the blosc compression library" ON)
option(BUILD_BENCHMARKS
    "Build benchmark programs form the blosc compression library" ON)


# testing
enable_testing()


# subdirectories
add_subdirectory(blosc)

if(BUILD_HDF5_FILTER)
    add_subdirectory(hdf5)
endif(BUILD_HDF5_FILTER)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

if(BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif(BUILD_BENCHMARKS)


# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


# packaging
include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR TGZ ZIP)
set(CPACK_SOURCE_GENERATOR TGZ ZIP)
set(CPACK_PACKAGE_VERSION_MAJOR ${BLOSC_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${BLOSC_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${BLOSC_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.rst")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A blocking, shuffling and lossless compression library")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/BLOSC.txt")
set(CPACK_SOURCE_IGNORE_FILES "/build.*;.*~;\\.git.*;\\.user$;\\.DS_Store")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_STRIP_FILES TRUE)

include(CPack)
