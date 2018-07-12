#.rst:
# FindLMDB
# --------
# Finds the LMDB library
#
# This will define the following variables:
#
# LMDB_FOUND - system has LMDB
# LMDB_INCLUDE_DIRS - the LMDB include directory
# LMDB_LIBRARIES - the LMDB libraries
#
# and the following imported targets:
#
#   LMDB::LMDB   - The LMDB library
#

if(ENABLE_INTERNAL_LMDB)
  include(ExternalProject)
  file(STRINGS "${CMAKE_SOURCE_DIR}/tools/depends/target/liblmdb/Makefile" VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" LMDB_VER "${VER}")

  # Allow user to override the download URL with a local tarball
  # Needed for offline build envs
  if(LMDB_URL)
    get_filename_component(LMDB_URL "${LMDB_URL}" ABSOLUTE)
  else()
    # TODO
    #set(LMDB_URL "http://mirrors.kodi.tv/build-deps/sources/lmdb-${LMDB_VER}.tar.gz")
    set(LMDB_URL "https://github.com/LMDB/lmdb/archive/LMDB_${LMDB_VER}.tar.gz")
  endif()
  if(VERBOSE)
    message(STATUS "LMDB_URL: ${LMDB_URL}")
  endif()

  set(LMDB_INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include"
    CACHE INTERNAL "LMDB include directory"
  )
  set(LMDB_LIBRARY
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lmdb${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CACHE INTERNAL "LMDB library"
  )

  externalproject_add(lmdb
    URL "${LMDB_URL}"
    DOWNLOAD_DIR "${TARBALL_DIR}"
    PREFIX "${CORE_BUILD_DIR}/lmdb"
    PATCH_COMMAND
      "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/tools/depends/target/liblmdb/CMakeLists.txt"
        <SOURCE_DIR>
    CMAKE_ARGS
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}"
      "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
      "${EXTRA_ARGS}"
    BUILD_BYPRODUCTS "${LMDB_LIBRARY}"
  )
  set_target_properties(lmdb PROPERTIES
    FOLDER "External Projects"
    INTERFACE_INCLUDE_DIRECTORIES "${LMDB_INCLUDE_DIR}"
  )
else()
  find_path(LMDB_INCLUDE_DIR NAMES lmdb.h)
  find_library(LMDB_LIBRARY NAMES lmdb)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB
  REQUIRED_VARS
    LMDB_INCLUDE_DIR
    LMDB_LIBRARY
  VERSION_VAR
    LMDB_VER
)

if(LMDB_FOUND)
  set(LMDB_INCLUDE_DIRS "${LMDB_INCLUDE_DIR}")
  set(LMDB_LIBRARIES "${LMDB_LIBRARY}")

  if(NOT TARGET LMDB::LMDB)
    add_library(LMDB::LMDB UNKNOWN IMPORTED)
    set_target_properties(LMDB::LMDB PROPERTIES
      FOLDER "External Projects"
      INTERFACE_INCLUDE_DIRECTORIES "${LMDB_INCLUDE_DIR}"
      IMPORTED_LOCATION "${LMDB_LIBRARY}"
    )
  endif()
endif()

mark_as_advanced(LMDB_INCLUDE_DIR)
mark_as_advanced(LMDB_LIBRARY)
