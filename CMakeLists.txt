# CMake build system for Blosc
# ============================
#
# Available options:
#
#   BUILD_STATIC: default ON
#       build the static version of the Blosc library
#   BUILD_HDF5_FILTER: default OFF
#       build the compression filter for the HDF5 library
#   BUILD_TESTS: default ON
#       build test programs and generates the "test" target
#   BUILD_BENCHMARKS: default ON
#       build the benchmark program
#   ENABLE_LZ4: default ON
#       enable the Lz4 compressor
#   ENABLE_SNAPPY: default ON
#       enable the Snappy compressor
#   ENABLE_ZLIB: default ON
#       enable the zlib compressor (if available)
#   TEST_INCLUDE_BENCH_SINGLE_1: default ON
#       add a test that runs the benchmark program passing "single" with 1 thread
#       as first parameter
#   TEST_INCLUDE_BENCH_SINGLE_N: default ON
#       add a test that runs the benchmark program passing "single" with all threads
#       as first parameter
#   TEST_INCLUDE_BENCH_SUITE: default OFF
#       add a test that runs the benchmark program passing "suite"
#       as first parameter
#   TEST_INCLUDE_BENCH_SUITE_PARALLEL: default OFF
#       add a test that runs the benchmark program passing "parallel"
#       as first parameter
#   TEST_INCLUDE_BENCH_HARDSUITE: default OFF
#       add a test that runs the benchmark program passing "hardsuite"
#       as first parameter
#   TEST_INCLUDE_BENCH_EXTREMESUITE: default OFF
#       add a test that runs the benchmark program passing "extremesuite"
#       as first parameter
#   TEST_INCLUDE_BENCH_DEBUGSUITE: default OFF
#       add a test that runs the benchmark program passing "debugsuite"
#       as first parameter
#   BENCH_N_THREADS: default 4
#       number of threads used to run the benchmark program during tests
#
# Components:
#
#    LIB: includes blosc.so
#    DEV: static includes blosc.a and blosc.h
#    HDF5_FILTER: includes blosc_filter.so
#    HDF5_FILTER_DEV: includes blosc_filter.h
#
# Making packages:
#
#   $ cmake <PATH_TO_SOURCES>
#   $ make package
#   $ make package_source
#
# Note: to include the Snappy source tarball in the archive run the
# following commands:
#
#   $ cmake <PATH_TO_SOURCES>
#   $ make Snappy-download
#   $ make package_source

cmake_minimum_required(VERSION 2.8)
project(blosc)

# parse the full version numbers from blosc.h
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/blosc/blosc.h _blosc_h_contents)
string(REGEX REPLACE ".*#define[ \t]+BLOSC_VERSION_MAJOR[ \t]+([0-9]+).*"
     "\\1" BLOSC_VERSION_MAJOR ${_blosc_h_contents})
string(REGEX REPLACE ".*#define[ \t]+BLOSC_VERSION_MINOR[ \t]+([0-9]+).*"
    "\\1" BLOSC_VERSION_MINOR ${_blosc_h_contents})
string(REGEX REPLACE ".*#define[ \t]+BLOSC_VERSION_RELEASE[ \t]+([0-9]+).*"
    "\\1" BLOSC_VERSION_PATCH ${_blosc_h_contents})
string(REGEX REPLACE ".*#define[ \t]+BLOSC_VERSION_STRING[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" BLOSC_VERSION_STRING ${_blosc_h_contents})

message(STATUS "Blosc version: " ${BLOSC_VERSION_STRING})

# options
option(BUILD_STATIC
    "Build a static version of the blosc library." ON)
option(BUILD_HDF5_FILTER
    "Build a blosc based compression filter for the HDF5 library" OFF)
option(BUILD_TESTS
    "Build test programs form the blosc compression library" ON)
option(BUILD_BENCHMARKS
    "Build benchmark programs form the blosc compression library" ON)

option(ENABLE_LZ4 "Enable the Lz4 compressor." ON)
option(ENABLE_SNAPPY "Enable the Snappy compressor." ON)
option(ENABLE_ZLIB "Enable the zlib compressor (if available)." ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(ENABLE_LZ4)
    find_package(LZ4)

    if(NOT LZ4_FOUND)
        include(Lz4ExternalProject)
    endif(NOT LZ4_FOUND)

    set(HAVE_LZ4 TRUE)
endif(ENABLE_LZ4)

if(ENABLE_SNAPPY)
    find_package(Snappy)

    if(NOT SNAPPY_FOUND)
        include(SnappyExternalProject)
    endif(NOT SNAPPY_FOUND)

    set(HAVE_SNAPPY TRUE)
endif(ENABLE_SNAPPY)

# import the ZLIB_ROOT environment variable to help finding the zlib library
if(ENABLE_ZLIB)
    set(ZLIB_ROOT $ENV{ZLIB_ROOT})

    find_package(ZLIB)

    if(ZLIB_FOUND)
        set(HAVE_ZLIB TRUE)
        set(BLOSC_ZLIB_VERSION_STRING ZLIB_VERSION_STRING)
    endif(ZLIB_FOUND)
endif(ENABLE_ZLIB)

# create the config.h file
configure_file ("blosc/config.h.in"  "blosc/config.h" )
# now make sure that you set the build directory on your "Include" path when compiling
include_directories("${PROJECT_BINARY_DIR}/blosc/")

# force the default build type to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)


# flags
# @TODO: set -Wall
# @NOTE: -O3 is enabled in Release mode (CMAKE_BUILD_TYPE="Release")

# Set the "-msse2" build flag only if the CMAKE_C_FLAGS is not already set.
# Probably "-msse2" should be appended to CMAKE_C_FLAGS_RELEASE.
if(CMAKE_C_COMPILER_ID STREQUAL GNU OR CMAKE_C_COMPILER_ID STREQUAL Clang)
     if(NOT CMAKE_C_FLAGS)
         set(CMAKE_C_FLAGS -msse2 CACHE STRING "C flags." FORCE)
     endif(NOT CMAKE_C_FLAGS)
endif(CMAKE_C_COMPILER_ID STREQUAL GNU OR CMAKE_C_COMPILER_ID STREQUAL Clang)

if(MSVC)
    if(NOT CMAKE_C_FLAGS)
        set(CMAKE_C_FLAGS "/Ox" CACHE STRING "C flags." FORCE)
    endif(NOT CMAKE_C_FLAGS)
endif(MSVC)

if(WIN32)
    # For some supporting headers
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/blosc")
endif(WIN32)


# subdirectories
add_subdirectory(blosc)

if(BUILD_HDF5_FILTER)
    add_subdirectory(hdf5)
endif(BUILD_HDF5_FILTER)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif(BUILD_TESTS)

if(BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif(BUILD_BENCHMARKS)


# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
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
set(CPACK_PACKAGE_VERSION ${BLOSC_STRING_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.rst")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "A blocking, shuffling and lossless compression library")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/BLOSC.txt")
set(CPACK_SOURCE_IGNORE_FILES
    "/build.*;.*~;\\\\.git.*;\\\\.DS_Store;\\\\.mailmap")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_STRIP_FILES TRUE)

include(CPack)
