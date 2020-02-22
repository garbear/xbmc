#.rst:
# FindLz4
# --------
# Finds the LZ4 library
#
# This will define the following variables:
#
#   LZ4_FOUND - system has LZ4
#   LZ4_INCLUDE_DIRS - the LZ4 include directory
#   LZ4_LIBRARIES - the LZ4 libraries
#
# and the following imported targets:
#
#   Lz4::Lz4 - The LZ4 library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET Lz4::Lz4)
  if(ENABLE_INTERNAL_LZ4)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC liblz4)

    SETUP_BUILD_VARS()

    set(LIBLZ4_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DBUILD_STATIC_LIBS=ON
                   -DLZ4_BUILD_CLI=OFF
                   -DLZ4_BUILD_LEGACY_LZ4C=OFF
                   -DLZ4_BUNDLED_MODE=OFF)

    set(LIBLZ4_SOURCE_SUBDIR build/cmake)

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBLZ4_INCLUDE_DIR NAMES lz4.h)
    find_library(LIBLZ4_LIBRARY NAMES lz4)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Lz4
    REQUIRED_VARS
      LIBLZ4_LIBRARY
      LIBLZ4_INCLUDE_DIR
    VERSION_VAR
      LIBLZ4_VERSION)

  if(LZ4_FOUND)
    set(LZ4_INCLUDE_DIRS "${LIBLZ4_INCLUDE_DIR}")
    set(LZ4_LIBRARIES "${LIBLZ4_LIBRARY}")

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Lz4::Lz4)

    add_library(Lz4::Lz4 UNKNOWN IMPORTED)

    set_target_properties(Lz4::Lz4 PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBLZ4_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBLZ4_INCLUDE_DIR}")

    if(TARGET liblz4)
      add_dependencies(Lz4::Lz4 liblz4)
    endif()
  else()
    if(LZ4_FIND_REQUIRED)
      message(FATAL_ERROR "LZ4 not found. Maybe use -DENABLE_INTERNAL_LZ4=ON")
    endif()
  endif()

  mark_as_advanced(LIBLZ4_INCLUDE_DIR)
  mark_as_advanced(LIBLZ4_LIBRARY)
endif()
