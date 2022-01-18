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
#   libqrencode::libqrencode   - The libqrencode library
#

if(ENABLE_INTERNAL_LIBQRENCODE)
  include(ExternalProject)
  file(STRINGS "${CMAKE_SOURCE_DIR}/tools/depends/target/libqrencode/Makefile" VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" LIBQRENCODE_VER "${VER}")

  # Allow user to override the download URL with a local tarball
  # Needed for offline build envs
  if(LIBQRENCODE_URL)
    get_filename_component(LIBQRENCODE_URL "${LIBQRENCODE_URL}" ABSOLUTE)
  else()
    # TODO
    #set(LIBQRENCODE_URL "http://mirrors.kodi.tv/build-deps/sources/libqrencode-${LIBQRENCODE_VER}.tar.gz")
    set(LIBQRENCODE_URL "https://fukuchi.org/works/qrencode/qrencode-${LIBQRENCODE_VER}.tar.gz")
  endif()
  if(VERBOSE)
    message(STATUS "LIBQRENCODE_URL: ${LIBQRENCODE_URL}")
  endif()

  if (WIN32)
    set(LIBQRENCODE_LIB_NAME qrencoded)
  else()
    set(LIBQRENCODE_LIB_NAME qrencode)
  endif()

  set(LIBQRENCODE_INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include"
    CACHE INTERNAL "libqrencode include directory"
  )
  set(LIBQRENCODE_LIBRARY
    "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBQRENCODE_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CACHE INTERNAL "libqrencode library"
  )

  externalproject_add(libqrencode
    URL "${LIBQRENCODE_URL}"
    DOWNLOAD_DIR "${TARBALL_DIR}"
    PREFIX "${CORE_BUILD_DIR}/libqrencode"
    CMAKE_ARGS
      "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}"
      "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
      -DWITH_TOOLS=OFF
      "${EXTRA_ARGS}"
    BUILD_BYPRODUCTS "${LIBQRENCODE_LIBRARY}"
  )
  set_target_properties(libqrencode PROPERTIES
    FOLDER "External Projects"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}"
  )
else()
  find_path(LIBQRENCODE_INCLUDE_DIR NAMES libqrencode.h)
  find_path(LIBQRENCODE_LIBRARY NAMES qrencode qrencoded)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libqrencode
  REQUIRED_VARS
    LIBQRENCODE_INCLUDE_DIR
    LIBQRENCODE_LIBRARY
  VERSION_VAR
    LIBQRENCODE_VE
)

if(LIBQRENCODE_FOUND)
  set(LIBQRENCODE_LIBRARIES "${LIBQRENCODE_LIBRARY}")
  set(LIBQRENCODE_INCLUDE_DIRS "${LIBQRENCODE_INCLUDE_DIR}")

  if(NOT TARGET libqrencode::libqrencode)
    add_library(libqrencode::libqrencode UNKNOWN IMPORTED)
    set_target_properties(libqrencode::libqrencode PROPERTIES
      FOLDER "External Projects"
      INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}"
      IMPORTED_LOCATION "${LIBQRENCODE_LIBRARY}"
    )
  endif()
endif()

mark_as_advanced(LIBQRENCODE_LIBRARY)
mark_as_advanced(LIBQRENCODE_INCLUDE_DIR)
