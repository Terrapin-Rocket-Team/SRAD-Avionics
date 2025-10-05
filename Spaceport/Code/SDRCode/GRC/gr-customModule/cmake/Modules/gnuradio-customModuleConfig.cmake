find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_CUSTOMMODULE gnuradio-customModule)

FIND_PATH(
    GR_CUSTOMMODULE_INCLUDE_DIRS
    NAMES gnuradio/customModule/api.h
    HINTS $ENV{CUSTOMMODULE_DIR}/include
        ${PC_CUSTOMMODULE_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_CUSTOMMODULE_LIBRARIES
    NAMES gnuradio-customModule
    HINTS $ENV{CUSTOMMODULE_DIR}/lib
        ${PC_CUSTOMMODULE_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-customModuleTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_CUSTOMMODULE DEFAULT_MSG GR_CUSTOMMODULE_LIBRARIES GR_CUSTOMMODULE_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_CUSTOMMODULE_LIBRARIES GR_CUSTOMMODULE_INCLUDE_DIRS)
