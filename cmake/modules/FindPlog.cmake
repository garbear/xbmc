# FindFmt
# -------
# Finds the plog library
#
# This will define the following variables::
#
# PLOG_FOUND - system has plog
# PLOG_INCLUDE_DIRS - the plog include directory
#
# and the following imported targets::
#
#   plog::plog   - The plog library
#

include(FindPackageHandleStandardArgs)

if(NOT TARGET plog::plog)
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC plog)

    SETUP_BUILD_VARS()

    set(CMAKE_ARGS -DPLOG_BUILD_SAMPLES=OFF
                   -DPLOG_INSTALL=ON
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endif()
endif()

include(FindPackageHandleStandardArgs)

find_path(PLOG_INCLUDE_DIR NAMES plog/Log.h)

find_package_handle_standard_args(Plog
  REQUIRED_VARS
    PLOG_INCLUDE_DIR)

if(PLOG_FOUND)
  set(PLOG_INCLUDE_DIRS ${PLOG_INCLUDE_DIR})

  if(NOT TARGET plog::plog)
    add_library(plog::plog INTERFACE IMPORTED)
    set_target_properties(plog::plog PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${PLOG_INCLUDE_DIR}")

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP plog::plog)
  endif()
endif()

mark_as_advanced(PLOG_INCLUDE_DIR)
