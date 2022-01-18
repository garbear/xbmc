#.rst:
# FindLibqrencode
# --------
# Find the libqrencode library and headers
#
# This will define the following variables:
#
#   LIBQRENCODE_FOUND - system has libqrencode library and headers
#   LIBQRENCODE_INCLUDE_DIRS - the libqrencode include directories
#   LIBQRENCODE_LIBRARIES - the libqrencode library directories
#
# and the following imported targets::
#
#   libqrencode::libqrencode - The libqrencode library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET libqrencode::libqrencode)
  if(ENABLE_INTERNAL_LIBQRENCODE)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libqrencode)

    SETUP_BUILD_VARS()

    set(LIBQRENCODE_VERSION ${${MODULE}_VER})

    # Debug postfix only used for windows
    if(WIN32 OR WINDOWS_STORE)
      set(LIBQRENCODE_DEBUG_POSTFIX "d")
    endif()

    set(CMAKE_ARGS -DWITH_TOOLS=NO)

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBQRENCODE_INCLUDE_DIR NAMES libqrencode.h)
    find_library(LIBQRENCODE_LIBRARY_RELEASE NAMES qrencode)
    find_library(LIBQRENCODE_LIBRARY_DEBUG NAMES qrencoded)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBQRENCODE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libqrencode
    REQUIRED_VARS
      LIBQRENCODE_LIBRARY
      LIBQRENCODE_INCLUDE_DIR
    VERSION_VAR
      LIBQRENCODE_VERSION
  )

  if(LIBQRENCODE_FOUND)
    set(LIBQRENCODE_INCLUDE_DIRS "${LIBQRENCODE_INCLUDE_DIR}")
    set(LIBQRENCODE_LIBRARIES "${LIBQRENCODE_LIBRARY}")

    add_library(libqrencode::libqrencode UNKNOWN IMPORTED)

    set_target_properties(libqrencode::libqrencode PROPERTIES
      FOLDER "External Projects"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}")

    if(LIBQRENCODE_LIBRARY_RELEASE)
      set_target_properties(libqrencode::libqrencode PROPERTIES
        IMPORTED_CONFIGURATIONS RELEASE
        IMPORTED_LOCATION_RELEASE "${LIBQRENCODE_LIBRARY_RELEASE}")
    endif()
    if(LIBQRENCODE_LIBRARY_DEBUG)
      set_target_properties(libqrencode::libqrencode PROPERTIES
        IMPORTED_CONFIGURATIONS DEBUG
        IMPORTED_LOCATION_DEBUG "${LIBQRENCODE_LIBRARY_DEBUG}")
    endif()

    if(TARGET libqrencode)
      add_dependencies(libqrencode::libqrencode libqrencode)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libqrencode::libqrencode)
  else()
    if(LIBQRENCODE_FIND_REQUIRED)
      message(FATAL_ERROR "libqrencode not found. Maybe use -DENABLE_INTERNAL_LIBQRENCODE=ON")
    endif()
  endif()

  mark_as_advanced(LIBQRENCODE_INCLUDE_DIR)
  mark_as_advanced(LIBQRENCODE_LIBRARY)
endif()
