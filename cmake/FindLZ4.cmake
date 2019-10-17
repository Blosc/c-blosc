find_path(LZ4_INCLUDE_DIR lz4.h)

find_library(LZ4_LIBRARY NAMES lz4)

if (LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
    set(LZ4_FOUND TRUE)
    message(STATUS "Found LZ4 library: ${LZ4_LIBRARY}")
    add_library(LZ4::LZ4 UNKNOWN IMPORTED)
    set_target_properties(LZ4::LZ4 PROPERTIES
        IMPORTED_LOCATION ${LZ4_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${LZ4_INCLUDE_DIR})
else ()
    message(STATUS "No LZ4 library found.  Using internal sources.")
endif ()