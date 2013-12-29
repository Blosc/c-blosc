find_path(SNAPPY_INCLUDE_DIR snappy-c.h)

find_library(SNAPPY_LIBRARY NAMES snappy)

# handle the QUIETLY and REQUIRED arguments and set SNAPPY_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SNAPPY
    REQUIRED_VARS SNAPPY_LIBRARY SNAPPY_INCLUDE_DIR)

if(SNAPPY_FOUND)
    set(HAVE_SNAPPY TRUE)
    message(STATUS "Have snappy: ${SNAPPY_INCLUDE_DIR} ${SNAPPY_LIBRARY}")
else ()
    message(STATUS "No snappy found")
endif ()
