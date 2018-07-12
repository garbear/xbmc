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
  macro(buildLMDB)
    set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

    set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
      "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/CMakeLists.txt"
      "${CMAKE_CURRENT_BINARY_DIR}/${CORE_BUILD_DIR}/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/src/${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME}/CMakeLists.txt"
    )

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC lmdb)

  SETUP_BUILD_VARS()

  # TODO: Check for existing LMDB. If version >= LMDB-VERSION file version, dont build
  if(ENABLE_INTERNAL_LMDB)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Build lib
      buildLMDB()
    endif()
  else()
    # Populate paths for find_package_handle_standard_args
    find_path(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR NAMES lmdb.h)
    find_library(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY NAMES lmdb)
    # TODO
    find_library(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE NAMES lmdb)
    find_library(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG NAMES lmdb)
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LMDB
    REQUIRED_VARS
      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
  )

  if(LMDB_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}"
    )

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}
        PROPERTIES
          IMPORTED_CONFIGURATIONS RELEASE
          IMPORTED_LOCATION_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE}"
      )
    endif()
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME}
        PROPERTIES
          IMPORTED_CONFIGURATIONS DEBUG
          IMPORTED_LOCATION_DEBUG "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG}"
      )
    endif()

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
        buildLMDB()
        set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(LMDB_FIND_REQUIRED)
      message(FATAL_ERROR "LMDB libraries were not found. You may want to try -DENABLE_INTERNAL_LMDB=ON")
    endif()
  endif()
endif()
