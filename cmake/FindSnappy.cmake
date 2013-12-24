find_path(SNAPPY_INCLUDE_DIR snappy-c.h NO_DEFAULT_PATH PATHS
  /usr/include
  /usr/local/include
)

find_library(SNAPPY_LIBRARY NAMES snappy NO_DEFAULT_PATH PATHS
    /usr/lib
    /usr/local/lib
    )

if (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)
    set(HAVE_SNAPPY TRUE)
    message(STATUS "Have snappy")
else ()
    message(STATUS "No snappy found")
endif ()

