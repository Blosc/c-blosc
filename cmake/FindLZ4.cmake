[==[
Find LZ4
Seaches system for both static and shared Lz4 binaries and headers
If successful, sets LZ4 targets LZ4::lz4_shared and LZ4::lz4_static
as relevant if one or both are found, and LZ4_INCLUDE_DIR.
LZ4_LIBRARIES is also set to a generator expression containing
one or both of the LZ4 library targets that will selectively
use the correct type based on the library you're building.
]==]
# Find the lz4 include dirs
find_path(LZ4_INCLUDE_DIR lz4.h)

# Find static lz4
# On Windows, the lz4 library is called liblz4.lib, which is not
# found by using the lz4 name.
# On native Windows lz4_static, on Darwin liblz4.a, on Linux liblz4.a
# cache default find library suffixes so subsequent find calls behave normally
set(ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
find_library(LZ4_STATIC_LIBRARY NAMES lz4 lz4_static liblz4)

# Find shared lz4
# On Windows lz4.lib, On Darwin lz4.dylib, On Linux, lz4.so
set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .so .dylib)
find_library(LZ4_SHARED_LIBRARY NAMES lz4 liblz4)
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)
    find_library(LZ4_DLL NAMES lz4)
endif()
# restore original find library behavior
set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

set(LZ4_FOUND_LIBRARIES)
if (LZ4_STATIC_LIBRARY OR LZ4_SHARED_LIBRARY)
    if(LZ4_STATIC_LIBRARY)
        set(LZ4_FOUND_LIBRARIES ${LZ4_FOUND_LIBRARIES} ${LZ4_STATIC_LIBRARY})
    endif()
    if(LZ4_SHARED_LIBRARY)
        set(LZ4_FOUND_LIBRARIES ${LZ4_FOUND_LIBRARIES} ${LZ4_SHARED_LIBRARY})
    endif()
    message(STATUS "Found LZ4 library: ${LZ4_FOUND_LIBRARIES}")
endif ()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LZ4
    REQUIRED_VARS LZ4_FOUND_LIBRARIES LZ4_INCLUDE_DIR
    REASON_FAILURE_MESSAGE "No LZ4 library found.  Using internal sources."
)

# Check if LZ4 was detected from a conig
# module first
if(NOT TARGET LZ4::lz4_shared)
    add_library(LZ4::lz4_shared SHARED IMPORTED)
    set_target_properties(LZ4::lz4_shared
        INTERFACE_INCLUDE_DIRECTORIES ${LZ4_INCLUDE_DIR}
    )
    if(WIN32)
        set_target_properties(LZ4::lz4_shared
            IMPORTED_IMPLIB ${LZ4_SHARED_LIBRARY}
            IMPORTED_LOCATION ${LZ4_DLL}
        )
    else()
        set_target_properties(LZ4::lz4_shared
            IMPORTED_LOCATION ${LZ4_SHARED_LIBRARY}
        )
    endif()
endif()
if(NOT TARGET LZ4::lz4_static)
    add_library(LZ4::lz4_static STATIC IMPORTED)
    set_target_properties(LZ4::lz4_static
        INTERFACE_INCLUDE_DIRECTORIES ${LZ4_INCLUDE_DIR}
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
        IMPORTED_LOCATION ${LZ4_STATIC_LIBRARY}
    )
endif()

# Set a genex to match the appropriate library type respective to the
# type of the LZ4 library
set(LZ4_LIBRARIES)
if(TARGET LZ4::lz4_shared)
    set(LZ4_LIBRARIES
        ${LZ4_LIBRARIES}
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:LZ4::lz4_shared>
    )
endif()
if(TARGET LZ4::lz4_static)
    set(LZ4_LIBRARIES
        ${LZ4_LIBRARIES}
        $<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>>:LZ4::lz4_static>
    )
endif()