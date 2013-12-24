find_path(ZLIB_INCLUDE_DIR zlib.h NO_DEFAULT_PATH PATHS
  /usr/include
  /usr/local/include
)

find_library(ZLIB_LIBRARY NAMES z NO_DEFAULT_PATH PATHS
    /usr/lib
    /usr/local/lib
    )

if (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    set(HAVE_ZLIB TRUE)
    message(STATUS "Have zlib")
else ()
    message(STATUS "No zlib found")
endif ()

