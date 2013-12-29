find_path(LZ4_INCLUDE_DIR lz4.h)

find_library(LZ4_LIBRARY NAMES lz4)

# handle the QUIETLY and REQUIRED arguments and set SNAPPY_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LZ4
    REQUIRED_VARS LZ4_LIBRARY LZ4_INCLUDE_DIR)

if(LZ4_FOUND)
    message(STATUS "Found LZ4 library: ${LZ4_INCLUDE_DIR} ${LZ4_LIBRARY}")
else ()
    message(STATUS "No lz4 found.")
endif ()
