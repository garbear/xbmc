#.rst:
# FindLMDB
# --------
# Finds the LMDB library
#
# This will define the following variables:
#
#   LMDB_FOUND - system has LMDB
#   LMDB_INCLUDE_DIRS - the LMDB include directory
#   LMDB_LIBRARIES - the LMDB libraries
#
# and the following imported targets:
#
#   LMDB::LMDB - The LMDB library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET LMDB::LMDB)
  if(ENABLE_INTERNAL_LMDB)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC liblmdb)

    SETUP_BUILD_VARS()

    set(LIBLMDB_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

    set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
      "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/CMakeLists.txt"
      "${CMAKE_CURRENT_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/CMakeLists.txt")

    BUILD_DEP_TARGET()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(LIBLMDB_INCLUDE_DIR NAMES lmdb.h)
    find_library(LIBLMDB_LIBRARY NAMES lmdb)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LMDB
    REQUIRED_VARS
      LIBLMDB_LIBRARY
      LIBLMDB_INCLUDE_DIR
    VERSION_VAR
      LIBLMDB_VERSION)

  if(LMDB_FOUND)
    set(LMDB_INCLUDE_DIRS "${LIBLMDB_INCLUDE_DIR}")
    set(LMDB_LIBRARIES "${LIBLMDB_LIBRARY}")

    add_library(LMDB::LMDB UNKNOWN IMPORTED)

    set_target_properties(LMDB::LMDB PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LIBLMDB_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBLMDB_INCLUDE_DIR}")

    if(TARGET liblmdb)
      add_dependencies(LMDB::LMDB liblmdb)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LMDB::LMDB)
  else()
    if(LMDB_FIND_REQUIRED)
      message(FATAL_ERROR "LMDB not found. Maybe use -DENABLE_INTERNAL_LMDB=ON")
    endif()
  endif()

  mark_as_advanced(LIBLMDB_INCLUDE_DIR)
  mark_as_advanced(LIBLMDB_LIBRARY)
endif()
