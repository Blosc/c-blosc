find_path(SNAPPY_INCLUDE_DIR snappy-c.h)

find_library(SNAPPY_LIBRARY NAMES snappy)

if (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)
    set(Snappy_FOUND TRUE)
    add_library(Snappy::snappy UNKNOWN IMPORTED)
    set_target_properties(Snappy::snappy PROPERTIES
        IMPORTED_LOCATION ${SNAPPY_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${SNAPPY_INCLUDE_DIR})
    message(STATUS "Found SNAPPY library: ${SNAPPY_LIBRARY}")
else ()
    message(STATUS "No snappy found.  Using internal sources.")
endif ()