find_path(LIZARD_INCLUDE_DIR lizard_common.h)

find_library(LIZARD_LIBRARY NAMES lizard)

if (LIZARD_INCLUDE_DIR AND LIZARD_LIBRARY)
    set(LIZARD_FOUND TRUE)
    message(STATUS "Found LIZARD library: ${LIZARD_LIBRARY}")
else ()
    message(STATUS "No LIZARD library found.  Using internal sources.")
endif ()
