#.rst:
# FindLMDB
# --------
# Finds the LMDB library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::LMDB - The LMDB library
#

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroLMDB)
    list(APPEND CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
    )

    set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
      "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/CMakeLists.txt"
      "${CMAKE_CURRENT_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/CMakeLists.txt"
    )

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC lmdb)

  SETUP_BUILD_VARS()

  # TODO: Is this needed?
  SETUP_FIND_SPECS()

  # TODO: Is this needed?
  SEARCH_EXISTING_PACKAGES()

  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_LMDB) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_LMDB) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR NAMES lmdb.h)
    find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY NAMES lmdb)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
    REQUIRED_VARS
      ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
      ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
  )

  if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
        IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
    )

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(LMDB_FIND_REQUIRED)
      message(FATAL_ERROR "LMDB libraries were not found. You may want to try -DENABLE_INTERNAL_LMDB=ON")
    endif()
  endif()
endif()
