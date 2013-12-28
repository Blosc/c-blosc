# a simple way to detect that we are using CMAKE
add_definitions(-DUSING_CMAKE)

# includes
if (HAVE_SNAPPY)
    include_directories( ${SNAPPY_INCLUDE_DIR} )
endif(HAVE_SNAPPY)
if (HAVE_ZLIB)
    include_directories( ${ZLIB_INCLUDE_DIR} )
endif(HAVE_ZLIB)

# library sources
set(SOURCES blosc.c blosclz.c shuffle.c lz4.c)
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
        set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
    endif(NOT Threads_FOUND)
else(WIN32)
    find_package(Threads REQUIRED)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)

if(HAVE_SNAPPY)
    set(LIBS ${LIBS} ${SNAPPY_LIBRARY})
endif(HAVE_SNAPPY)
if(HAVE_ZLIB)
    set(LIBS ${LIBS} ${ZLIB_LIBRARY})
endif(HAVE_ZLIB)


# targets
add_library(blosc_shared SHARED ${SOURCES})
set_target_properties(blosc_shared PROPERTIES OUTPUT_NAME blosc)
set_target_properties(blosc_shared PROPERTIES
        VERSION ${version_string}
        SOVERSION ${version_string}
    )
target_link_libraries(blosc_shared ${LIBS})

if(BUILD_STATIC)
    add_library(blosc_static STATIC ${SOURCES})
    set_target_properties(blosc_static PROPERTIES OUTPUT_NAME blosc)
    target_link_libraries(blosc_static ${LIBS})
endif(BUILD_STATIC)


# install
install(FILES blosc.h DESTINATION include COMPONENT DEV)
install(TARGETS blosc_shared DESTINATION ${lib_dir} COMPONENT LIB)
if(BUILD_STATIC)
    install(TARGETS blosc_static DESTINATION ${lib_dir} COMPONENT DEV)
endif(BUILD_STATIC)
