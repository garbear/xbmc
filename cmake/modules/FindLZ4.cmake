#.rst:
# FindLZ4
# --------
# Finds the LZ4 library
#
# This will define the following variables:
#
# LZ4_FOUND - system has LZ4
# LZ4_INCLUDE_DIRS - the LZ4 include directory
# LZ4_LIBRARIES - the LZ4 libraries
#
# and the following imported targets:
#
#   LZ4::LZ4   - The LZ4 library
#

if(ENABLE_INTERNAL_LZ4)
  include(ExternalProject)
  file(STRINGS "${CMAKE_SOURCE_DIR}/tools/depends/target/liblz4/Makefile" VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" LZ4_VER "${VER}")

  # Allow user to override the download URL with a local tarball
  # Needed for offline build envs
  if(LZ4_URL)
    get_filename_component(LZ4_URL "${LZ4_URL}" ABSOLUTE)
  else()
    # TODO
    #set(LZ4_URL "http://mirrors.kodi.tv/build-deps/sources/lz4-${LZ4_VER}.tar.gz")
    set(LZ4_URL "https://github.com/lz4/lz4/archive/v${LZ4_VER}.tar.gz")
  endif()
  if(VERBOSE)
    message(STATUS "LZ4_URL: ${LZ4_URL}")
  endif()

  set(LZ4_INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include"
    CACHE INTERNAL "LZ4 include directory"
  )
  set(LZ4_LIBRARY
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lz4${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CACHE INTERNAL "LZ4 library"
  )

  externalproject_add(lz4
    URL "${LZ4_URL}"
    DOWNLOAD_DIR "${TARBALL_DIR}"
    PREFIX "${CORE_BUILD_DIR}/lz4"
    CONFIGURE_COMMAND
      "${CMAKE_COMMAND}"
        "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}"
        "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
        -DLZ4_BUNDLED_MODE=OFF
        -DLZ4_BUILD_CLI=OFF
        -DLZ4_BUILD_LEGACY_LZ4C=OFF
        ${EXTRA_ARGS}
        <SOURCE_DIR>/build/cmake
    BUILD_BYPRODUCTS "${LZ4_LIBRARY}"
  )
  set_target_properties(lz4 PROPERTIES
    FOLDER "External Projects"
    INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
  )
else()
  find_path(LZ4_INCLUDE_DIR
    NAMES lz4.h
    DOC "LZ4 include directory"
  )
  find_library(LZ4_LIBRARY
    NAMES lz4 lz4
    DOC "LZ4 library"
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4
  REQUIRED_VARS
    LZ4_INCLUDE_DIR
    LZ4_LIBRARY
  VERSION_VAR
    LZ4_VER
)

if(LZ4_FOUND)
  set(LZ4_INCLUDE_DIRS "${LZ4_INCLUDE_DIR}")
  set(LZ4_LIBRARIES "${LZ4_LIBRARY}")

  if(NOT TARGET LZ4::LZ4)
    add_library(LZ4::LZ4 UNKNOWN IMPORTED)
    set_target_properties(LZ4::LZ4 PROPERTIES
      FOLDER "External Projects"
      IMPORTED_LOCATION "${LZ4_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
    )
  endif()
endif()
