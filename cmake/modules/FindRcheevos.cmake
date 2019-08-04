# FindRcheevos
# --------
# Find the RetroAchievements library and headers
#
# This will define the following variables:
#
# RCHEEVOS_FOUND - system has rcheevos
# RCHEEVOS_INCLUDE_DIRS - the rcheevos include directory
# RCHEEVOS_LIBRARY_DIRS - the rcheevos library directory

if(ENABLE_INTERNAL_RCHEEVOS)
  include(ExternalProject)
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/rcheevos/Makefile VER REGEX "^[ ]*VERSION[ ]*=.+$")
  string(REGEX REPLACE "^[ ]*VERSION[ ]*=[ ]*" "" RCHEEVOS_VER "${VER}")

  # Allow user to override the download URL with a local tarball
  # Needed for offline build envs
  if(RCHEEVOS_URL)
    get_filename_component(RCHEEVOS_URL "${RCHEEVOS_URL}" ABSOLUTE)
  else()
    # TODO
    #set(RCHEEVOS_URL http://mirrors.kodi.tv/build-deps/sources/rcheevos-${RCHEEVOS_VER}.tar.gz)
    set(RCHEEVOS_URL https://github.com/RetroAchievements/rcheevos/archive/v${RCHEEVOS_VER}.tar.gz)
  endif()
  if(VERBOSE)
    message(STATUS "RCHEEVOS_URL: ${RCHEEVOS_URL}")
  endif()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(RCHEEVOS_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/librcheevoslib.a)
  set(RCHEEVOS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  externalproject_add(rcheevos
                      URL ${RCHEEVOS_URL}
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/rcheevos
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 "${EXTRA_ARGS}"
                      PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                                    ${CMAKE_SOURCE_DIR}/tools/depends/target/rcheevos/CMakeLists.txt
                                    <SOURCE_DIR># &&
                                    #${CMAKE_COMMAND} -E copy
                                    #${CMAKE_SOURCE_DIR}/tools/depends/target/rcheevos/FindLua.cmake
                                    #<SOURCE_DIR>
                      BUILD_BYPRODUCTS ${RCHEEVOS_LIBRARY})
  set_target_properties(rcheevos PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Rcheevos
                                    REQUIRED_VARS RCHEEVOS_LIBRARY RCHEEVOS_INCLUDE_DIR
                                    VERSION_VAR RCHEEVOS_VER)

else()
  find_path(RCHEEVOS_INCLUDE_DIR NAMES rcheevos.h)
  find_library(RCHEEVOS_LIBRARY NAMES rcheevoslib)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Rcheevos
                                    REQUIRED_VARS RCHEEVOS_LIBRARY RCHEEVOS_INCLUDE_DIR)
endif()

if(RCHEEVOS_FOUND)
  set(RCHEEVOS_INCLUDE_DIRS ${RCHEEVOS_INCLUDE_DIR})
  set(RCHEEVOS_LIBRARIES ${RCHEEVOS_LIBRARY})

  if(NOT TARGET rcheevos)
    add_library(rcheevos UNKNOWN IMPORTED)
    set_target_properties(rcheevos PROPERTIES FOLDER "External Projects")
  endif()
endif()

mark_as_advanced(RCHEEVOS_LIBRARY RCHEEVOS_INCLUDE_DIR)
