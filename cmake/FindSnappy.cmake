find_path(SNAPPY_INCLUDE_DIR snappy-c.h)

find_library(SNAPPY_LIBRARY NAMES snappy)

if (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)
    set(HAVE_SNAPPY TRUE)
    message(STATUS "Have snappy: ${SNAPPY_INCLUDE_DIR} ${SNAPPY_LIBRARY}")
else ()
    message(STATUS "No snappy found")
endif ()

