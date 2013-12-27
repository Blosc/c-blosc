find_path(ZLIB_INCLUDE_DIR zlib.h NO_DEFAULT_PATH PATHS
    /usr/include
    /usr/local/include
    # /libs/zlib128/include  # on Windows put here your zlib dir
)

find_library(ZLIB_LIBRARY NAMES zdll z zlib NO_DEFAULT_PATH PATHS
    /usr/lib
    /usr/local/lib
    # /libs/zlib128/lib    # on Windows put here your zlib dir
)

if (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    set(HAVE_ZLIB TRUE)
    message(STATUS "Have zlib")
else ()
    message(STATUS "No zlib found")
endif ()
