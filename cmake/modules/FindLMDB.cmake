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
    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/lmdb/001-win-uwp.patch")
    generate_patchcommand("${patches}")
    unset(patches)

    list(APPEND PATCH_COMMAND COMMAND ${CMAKE_COMMAND} -E copy
      "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/CMakeLists.txt"
      <SOURCE_DIR>/CMakeLists.txt
    )

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
      # RelWithDebInfo uses the release CRT.
      set(BUILD_BYPRODUCTS "${DEPENDS_PATH}/lib/lmdb$<$<CONFIG:Debug>:d>.lib")
    endif()

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER})

    set(LMDB_INCLUDE_DIR ${DEPENDS_PATH}/include)
    set(LMDB_VERSION ${LMDB_VER})
    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC lmdb)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  set(LMDB_VERSION ${lmdb_VERSION})
  if(NOT TARGET lmdb::lmdb AND NOT TARGET PkgConfig::lmdb)
    set(lmdb_FOUND FALSE)
    find_path(LMDB_INCLUDE_DIR NAMES lmdb.h HINTS ${DEPENDS_PATH}/include)
    find_library(LMDB_LIBRARY_RELEASE NAMES lmdb HINTS ${DEPENDS_PATH}/lib)
    if(WIN32 OR WINDOWS_STORE)
      find_library(LMDB_LIBRARY_DEBUG NAMES lmdbd HINTS ${DEPENDS_PATH}/lib)
    endif()

    if(LMDB_INCLUDE_DIR AND (LMDB_LIBRARY_RELEASE OR LMDB_LIBRARY_DEBUG))
      file(STRINGS "${LMDB_INCLUDE_DIR}/lmdb.h" _lmdb_version_lines
           REGEX "^#define MDB_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+")
      foreach(_part MAJOR MINOR PATCH)
        string(REGEX MATCH "MDB_VERSION_${_part}[ \t]+([0-9]+)" _match "${_lmdb_version_lines}")
        set(_lmdb_${_part} "${CMAKE_MATCH_1}")
      endforeach()
      set(LMDB_VERSION "${_lmdb_MAJOR}.${_lmdb_MINOR}.${_lmdb_PATCH}")
      set(lmdb_FOUND TRUE)
    endif()
  endif()

  if(ENABLE_INTERNAL_LMDB AND
     (NOT lmdb_FOUND OR LMDB_VERSION VERSION_LESS LMDB_VER OR LMDB_FORCE_BUILD OR
      (CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR
      CORE_SYSTEM_NAME STREQUAL freebsd))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
    REQUIRED_VARS
      lmdb_FOUND
    VERSION_VAR LMDB_VERSION
  )

  if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    elseif(TARGET lmdb::lmdb)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS lmdb::lmdb)
    elseif(TARGET PkgConfig::lmdb)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::lmdb)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LMDB_INCLUDE_DIR}")

      if(LMDB_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
          IMPORTED_CONFIGURATIONS RELEASE
          IMPORTED_LOCATION_RELEASE "${LMDB_LIBRARY_RELEASE}")
      endif()
      if(LMDB_LIBRARY_DEBUG)
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
          IMPORTED_LOCATION_DEBUG "${LMDB_LIBRARY_DEBUG}")
      endif()
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    if(ENABLE_INTERNAL_LMDB)
      ADD_MULTICONFIG_BUILDMACRO()
    endif()
  else()
    if(LMDB_FIND_REQUIRED)
      message(FATAL_ERROR "LMDB libraries were not found. You may want to try -DENABLE_INTERNAL_LMDB=ON")
    endif()
  endif()
endif()
