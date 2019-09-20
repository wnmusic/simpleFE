INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SIMPLEFE simplefe)

FIND_PATH(
    SIMPLEFE_INCLUDE_DIRS
    NAMES simplefe/api.h
    HINTS $ENV{SIMPLEFE_DIR}/include
        ${PC_SIMPLEFE_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SIMPLEFE_LIBRARIES
    NAMES gnuradio-simplefe
    HINTS $ENV{SIMPLEFE_DIR}/lib
        ${PC_SIMPLEFE_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SIMPLEFE DEFAULT_MSG SIMPLEFE_LIBRARIES SIMPLEFE_INCLUDE_DIRS)
MARK_AS_ADVANCED(SIMPLEFE_LIBRARIES SIMPLEFE_INCLUDE_DIRS)

