# FindFmt
# -------
# Finds the libjuice library
#
# This will define the following variables::
#
# LIBJUICE_FOUND - system has libjuice
# LIBJUICE_INCLUDE_DIRS - the libjuice include directory
# LIBJUICE_LIBRARIES - the libjuice libraries
#
# and the following imported targets::
#
#   Libjuice::Libjuice   - The libjuice library
#

include(FindPackageHandleStandardArgs)

if(NOT TARGET Libjuice::Libjuice)
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libjuice)

    SETUP_BUILD_VARS()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Don-t-exclude-static-library.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DNO_TESTS=ON
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endif()
endif()

find_path(LIBJUICE_INCLUDE_DIR juice/juice.h)
find_library(LIBJUICE_LIBRARY NAMES juice)

find_package_handle_standard_args(Libjuice
  REQUIRED_VARS
    LIBJUICE_LIBRARY
    LIBJUICE_INCLUDE_DIR)

if(LIBJUICE_FOUND)
  set(LIBJUICE_INCLUDE_DIRS ${LIBJUICE_INCLUDE_DIR})
  set(LIBJUICE_LIBRARIES ${LIBJUICE_LIBRARY})

  if(NOT TARGET Libjuice::Libjuice)
    add_library(Libjuice::Libjuice UNKNOWN IMPORTED)

    set_target_properties(Libjuice::Libjuice PROPERTIES
        IMPORTED_LOCATION "${LIBJUICE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBJUICE_INCLUDE_DIR}")

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Libjuice::Libjuice)
  endif()
endif()

mark_as_advanced(LIBJUICE_INCLUDE_DIR LIBJUICE_LIBRARY)
