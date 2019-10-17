find_path(ZSTD_INCLUDE_DIR zstd.h)

find_library(ZSTD_LIBRARY NAMES zstd)

if (ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY)
    set(Zstd_FOUND TRUE)
    add_library(Zstd::Zstd UNKNOWN IMPORTED)
    set_target_properties(Zstd::Zstd PROPERTIES 
        IMPORTED_LOCATION ${ZSTD_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${ZSTD_INCLUDE_DIR})
    message(STATUS "Found Zstd library: ${ZSTD_LIBRARY}")
else ()
    message(STATUS "No Zstd library found.  Using internal sources.")
endif ()