# library sources
set(SOURCES blosc.c blosclz.c shuffle.c)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
if(WIN32)
    # try to use the system library
    find_package(Threads)
    if(NOT Threads_FOUND)
        message(STATUS "using the internal pthead library for win32 systems.")
        set(SOURCES ${SOURCES} win32/pthread.c)
        include_directories("${PROJECT_SOURCE_DIR}/win32")
    endif(NOT Threads_FOUND)
else(WIN32)
    find_package(Threads REQUIRED)
endif(WIN32)

# options
option(BUILD_SHARED "Build a share version of the blosc library." ON)


# flags
# @TODO:
# -O3 -Wall -fPIC -msse2


# targets
add_library(blosc_static STATIC ${SOURCES})
set_target_properties(blosc_static PROPERTIES OUTPUT_NAME blosc)
#target_link_library(blosc_static ${CMAKE_THREAD_LIBS_INIT})

if(BUILD_SHARED)
    add_library(blosc_shared SHARED ${SOURCES})
	set_target_properties(blosc_shared PROPERTIES OUTPUT_NAME blosc)
    if(Threads_FOUND)
        target_link_libraries(blosc_shared ${CMAKE_THREAD_LIBS_INIT})
    endif(Threads_FOUND)
endif(BUILD_SHARED)


# install
install(FILES blosc.h DESTINATION include)
install(TARGETS blosc_static DESTINATION lib)
if(BUILD_SHARED)
    install(TARGETS blosc_shared DESTINATION lib)
endif(BUILD_SHARED)
