#.rst:
# FindLibqrencode
# --------
# Find the libqrencode library and headers
#
# This will define the following variables:
#
# LIBQRENCODE_FOUND - system has libqrencode library and headers
# LIBQRENCODE_INCLUDE_DIRS - the libqrencode include directories
# LIBQRENCODE_LIBRARIES - the libqrencode library directories
#
# and the following imported targets::
#
#   libqrencode::libqrencode - The libqrencode library
#

include(FindPackageHandleStandardArgs)

if (NOT TARGET libqrencode::libqrencode)
  if(ENABLE_INTERNAL_LIBQRENCODE)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libqrencode)

    SETUP_BUILD_VARS()

    set(LIBQRENCODE_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DWITH_TOOLS=NO
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endif()
endif()

find_path(LIBQRENCODE_INCLUDE_DIR NAMES libqrencode.h)
find_path(LIBQRENCODE_LIBRARY NAMES qrencode qrencoded)

find_package_handle_standard_args(Libqrencode
  REQUIRED_VARS
    LIBQRENCODE_INCLUDE_DIR
    LIBQRENCODE_LIBRARY
  VERSION_VAR
    LIBQRENCODE_VERSION
)

if(LIBQRENCODE_FOUND)
  set(LIBQRENCODE_INCLUDE_DIRS "${LIBQRENCODE_INCLUDE_DIR}")
  set(LIBQRENCODE_LIBRARIES "${LIBQRENCODE_LIBRARY}")

  if(NOT TARGET libqrencode::libqrencode)
    add_library(libqrencode::libqrencode UNKNOWN IMPORTED)

    set_target_properties(libqrencode::libqrencode PROPERTIES
      FOLDER "External Projects"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}"
      IMPORTED_LOCATION "${LIBQRENCODE_LIBRARY}"
    )

    if(TARGET libqrencode)
      add_dependencies(libqrencode::libqrencode libqrencode)
    endif()
  endif()

  # Add dependency to libkodi to build
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libqrencode::libqrencode)
endif()

mark_as_advanced(LIBQRENCODE_LIBRARY)
mark_as_advanced(LIBQRENCODE_INCLUDE_DIR)
