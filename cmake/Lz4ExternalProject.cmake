# If LZ4 is not found in the system paths then download and build
# a copy of the library that will be linked statically.

message(STATUS "Using internal LZ4")

if(NOT EXTERN_PREFIX)
    set(EXTERN_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/extern)
endif(NOT EXTERN_PREFIX)

include(ExternalProject)
ExternalProject_Add(
    Lz4
    PREFIX ${EXTERN_PREFIX}

    #--Download step--------------
    DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern
    URL https://dl.dropboxusercontent.com/u/59565338/LZ4/lz4-r109.tar.gz
    URL_HASH SHA1=879DFCA273FE9D26C267A115FB023418072AEA06

    #--Update/Patch step----------
    #PATCH_COMMAND ...

    #--Configure step-------------
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND}
        -DCMAKE_INSTALL_PREFIX=${EXTERN_PREFIX}
        -DCMAKE_C_FLAGS=-fPIC
        -DBUILD_SHARED_LIBS=TRUE  # disables lz4c32
        ${CMAKE_CURRENT_BINARY_DIR}/extern/src/Lz4/cmake
    #CMAKE_ARGS
    #    -DCMAKE_INSTALL_PREFIX=${EXTERN_PREFIX}
    #    -DBUILD_SHARED_LIBS=TRUE  # disables lz4c32
    #    -DCMAKE_C_FLAGS=-fPIC

    #--Build step-----------------
    BINARY_DIR ${EXTERN_PREFIX}/src/Lz4/cmake
    #BUILD_COMMAND make

    #--Test step------------------
    #TEST_COMMAND make test

    #--Custom targets-------------
    STEP_TARGETS download configure build test install
)

set(LZ4_INCLUDE_DIR ${EXTERN_PREFIX}/include)
set(LZ4_LIBRARY ${EXTERN_PREFIX}/lib/liblz4.a)
set(LZ4_FOUND TRUE)
