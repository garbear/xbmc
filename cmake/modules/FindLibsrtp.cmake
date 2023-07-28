# FindFmt
# -------
# Finds the libsrtp library
#
# This will define the following variables::
#
# LIBSRTP_FOUND - system has libsrtp
# LIBSRTP_INCLUDE_DIRS - the libsrtp include directory
# LIBSRTP_LIBRARIES - the libsrtp libraries
#
# and the following imported targets::
#
#   libsrtp::libsrtp   - The libsrtp library
#

include(FindPackageHandleStandardArgs)

if(NOT TARGET libsrtp::libsrtp)
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libsrtp)

    SETUP_BUILD_VARS()

    set(CMAKE_ARGS -DENABLE_OPENSSL=ON
                   -DTEST_APPS=OFF
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endif()
endif()

find_path(LIBSRTP_INCLUDE_DIR NAMES srtp2/srtp.h)
find_library(LIBSRTP_LIBRARY NAMES srtp2)

find_package_handle_standard_args(Libsrtp
  REQUIRED_VARS
    LIBSRTP_LIBRARY
    LIBSRTP_INCLUDE_DIR)

if(LIBSRTP_FOUND)
  set(LIBSRTP_INCLUDE_DIRS ${LIBSRTP_INCLUDE_DIR})
  set(LIBSRTP_LIBRARIES ${LIBSRTP_LIBRARY})

  if(NOT TARGET libsrtp::libsrtp)
    add_library(libsrtp::libsrtp UNKNOWN IMPORTED)

    set_target_properties(libsrtp::libsrtp PROPERTIES
        IMPORTED_LOCATION "${LIBSRTP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSRTP_INCLUDE_DIR}")

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libsrtp::libsrtp)
  endif()
endif()

mark_as_advanced(LIBSRTP_INCLUDE_DIR LIBSRTP_LIBRARY)
