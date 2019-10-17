# a simple way to detect that we are using CMAKE
add_definitions(-DUSING_CMAKE)

# Hide symbols by default unless they're specifically exported.
# This makes it easier to keep the set of exported symbols the
# same across all compilers/platforms.
set(CMAKE_C_VISIBILITY_PRESET hidden)


# library sources
set(SOURCES blosc.c blosclz.c fastcopy.c shuffle-generic.c bitshuffle-generic.c
        blosc-common.h blosc-export.h)
if(COMPILER_SUPPORT_SSE2)
    message(STATUS "Adding run-time support for SSE2")
    set(SOURCES ${SOURCES} shuffle-sse2.c bitshuffle-sse2.c)
endif(COMPILER_SUPPORT_SSE2)
if(COMPILER_SUPPORT_AVX2)
    message(STATUS "Adding run-time support for AVX2")
    set(SOURCES ${SOURCES} shuffle-avx2.c bitshuffle-avx2.c)
endif(COMPILER_SUPPORT_AVX2)
set(SOURCES ${SOURCES} shuffle.c)

# library install directory
set(lib_dir lib${LIB_SUFFIX})
set(version_string ${BLOSC_VERSION_MAJOR}.${BLOSC_VERSION_MINOR}.${BLOSC_VERSION_PATCH})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
if(WIN32)
    # try to use the system library
    find_package(Threads)
    if(NOT Threads_FOUND)
        message(STATUS "using the internal pthread library for win32 systems.")
        set(SOURCES ${SOURCES} win32/pthread.c)
    else(NOT Threads_FOUND)
        list(APPEND PUBLIC_LIBS Threads::Threads)
        set(PUBLIC_PACKAGES "${PUBLIC_PACKAGES};Threads" CACHE INTERNAL "")
    endif(NOT Threads_FOUND)
else(WIN32)
    find_package(Threads REQUIRED)
    list(APPEND PUBLIC_LIBS Threads::Threads)
    set(PUBLIC_PACKAGES "${PUBLIC_PACKAGES};Threads" CACHE INTERNAL "")
endif(WIN32)

# targets
if (BUILD_SHARED)
    add_library(blosc_shared SHARED ${SOURCES})
    set_target_properties(blosc_shared PROPERTIES OUTPUT_NAME blosc)
    set_target_properties(blosc_shared PROPERTIES
            VERSION ${version_string}
            SOVERSION 1  # Change this when an ABI change happens
        )
    set_property(
        TARGET blosc_shared
        APPEND PROPERTY COMPILE_DEFINITIONS BLOSC_SHARED_LIBRARY)
endif()

# Based on the target architecture and hardware features supported
# by the C compiler, set hardware architecture optimization flags
# for specific shuffle implementations.
if(COMPILER_SUPPORT_SSE2)
    if (MSVC)
        # MSVC targets SSE2 by default on 64-bit configurations, but not 32-bit configurations.
        if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
            set_source_files_properties(shuffle-sse2.c bitshuffle-sse2.c PROPERTIES COMPILE_FLAGS "/arch:SSE2")
        endif (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    else (MSVC)
        set_source_files_properties(shuffle-sse2.c bitshuffle-sse2.c PROPERTIES COMPILE_FLAGS -msse2)
    endif (MSVC)

    # Define a symbol for the shuffle-dispatch implementation
    # so it knows SSE2 is supported even though that file is
    # compiled without SSE2 support (for portability).
    set_property(
        SOURCE shuffle.c
        APPEND PROPERTY COMPILE_DEFINITIONS SHUFFLE_SSE2_ENABLED)
endif(COMPILER_SUPPORT_SSE2)
if(COMPILER_SUPPORT_AVX2)
    if (MSVC)
        set_source_files_properties(shuffle-avx2.c bitshuffle-avx2.c
                PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    else (MSVC)
        set_source_files_properties(shuffle-avx2.c bitshuffle-avx2.c
                PROPERTIES COMPILE_FLAGS -mavx2)
    endif (MSVC)

    # Define a symbol for the shuffle-dispatch implementation
    # so it knows AVX2 is supported even though that file is
    # compiled without AVX2 support (for portability).
    set_property(
        SOURCE shuffle.c
        APPEND PROPERTY COMPILE_DEFINITIONS SHUFFLE_AVX2_ENABLED)
endif(COMPILER_SUPPORT_AVX2)

# When the option has been selected to compile the test suite,
# compile an additional version of blosc_shared which exports
# some normally-hidden symbols (to facilitate unit testing).
if (BUILD_TESTS)
    add_library(blosc_shared_testing SHARED ${SOURCES})
    set_target_properties(blosc_shared_testing PROPERTIES OUTPUT_NAME blosc_testing)
    set_property(
        TARGET blosc_shared_testing
        APPEND PROPERTY COMPILE_DEFINITIONS BLOSC_SHARED_LIBRARY)
    set_property(
        TARGET blosc_shared_testing
        APPEND PROPERTY COMPILE_DEFINITIONS BLOSC_TESTING)
    # TEMP : CMake doesn't automatically add -lpthread here like it does
    # for the blosc_shared target. Force it for now.
    if(UNIX)
        set_property(
            TARGET blosc_shared_testing
            APPEND PROPERTY LINK_FLAGS "-lpthread")
    endif()
endif()

add_library(blosc INTERFACE)

if (BUILD_SHARED)
    target_link_libraries(blosc_shared PUBLIC ${PUBLIC_LIBS})
    target_link_libraries(blosc_shared PRIVATE ${PRIVATE_LIBS})
    target_include_directories(blosc_shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_link_libraries(blosc INTERFACE blosc_shared)
endif()

if (BUILD_TESTS)
    target_link_libraries(blosc_shared_testing ${PUBLIC_LIBS} ${PRIVATE_LIBS})
    target_include_directories(blosc_shared_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BUILD_STATIC)
    add_library(blosc_static STATIC ${SOURCES})
    set_target_properties(blosc_static PROPERTIES OUTPUT_NAME blosc)
    if (MSVC)
        set_target_properties(blosc_static PROPERTIES PREFIX lib)
    endif()
    target_link_libraries(blosc_static PUBLIC ${PUBLIC_LIBS})
    target_link_libraries(blosc_static PRIVATE ${PRIVATE_LIBS})
    target_include_directories(blosc_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    if (NOT BUILD_SHARED)
        target_link_libraries(blosc INTERFACE blosc_static)
    endif()
endif(BUILD_STATIC)


# install
if(BLOSC_INSTALL)
    install(FILES blosc.h blosc-export.h DESTINATION include COMPONENT DEV)
    set(_inst_libs "blosc")
    if(BUILD_SHARED)
        list(APPEND _inst_libs blosc_shared)
    endif(BUILD_SHARED)
    if(BUILD_STATIC)
        list(APPEND _inst_libs blosc_static)
    endif(BUILD_STATIC)

    install(TARGETS ${_inst_libs} 
            EXPORT BloscTargets 
            LIBRARY DESTINATION ${lib_dir} 
            ARCHIVE DESTINATION ${lib_dir} 
            RUNTIME DESTINATION bin 
            COMPONENT DEV
            INCLUDES DESTINATION include)
endif(BLOSC_INSTALL)
