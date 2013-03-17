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
    else(NOT Threads_FOUND)
        set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
    endif(NOT Threads_FOUND)
else(WIN32)
    find_package(Threads REQUIRED)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)


# targets
add_library(blosc_shared SHARED ${SOURCES})
set_target_properties(blosc_shared PROPERTIES OUTPUT_NAME blosc)
target_link_libraries(blosc_shared ${LIBS})

if(BUILD_STATIC)
    add_library(blosc_static STATIC ${SOURCES})
    set_target_properties(blosc_static PROPERTIES OUTPUT_NAME blosc)
    target_link_libraries(blosc_static ${LIBS})
endif(BUILD_STATIC)


# install
install(FILES blosc.h DESTINATION include COMPONENT DEV)
install(TARGETS blosc_shared DESTINATION lib COMPONENT LIB)
if(BUILD_STATIC)
    install(TARGETS blosc_static DESTINATION lib COMPONENT DEV)
endif(BUILD_STATIC)
