#.rst:
# FindLibqrencode
# --------
# Find the libqrencode library and headers
#
#   ${APP_NAME_LC}::libqrencode - The libqrencode library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibqrencode)
    set(LIBQRENCODE_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS -DWITH_TOOLS=NO)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libqrencode)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libqrencode. If version >= LIBQRENCODE-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBQRENCODE)
    # Build lib
    buildLibqrencode()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(LIBQRENCODE)
  unset(LIBQRENCODE_LIBRARIES)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libqrencode
                                    REQUIRED_VARS LIBQRENCODE_LIBRARY LIBQRENCODE_INCLUDE_DIR
                                    VERSION_VAR LIBQRENCODE_VERSION)
  if(Libqrencode_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(LIBQRENCODE_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${LIBQRENCODE_LIBRARY_RELEASE}")
    endif()
    if(LIBQRENCODE_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${LIBQRENCODE_LIBRARY_DEBUG}")
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBQRENCODE_INCLUDE_DIR}")
    if(TARGET libqrencode)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libqrencode)
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
      if(NOT TARGET libqrencode)
        buildLibqrencode()
        set_target_properties(libqrencode PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends libqrencode)
    endif()
  else()
    if(Libqrencode_FIND_REQUIRED)
      message(FATAL_ERROR "libqrencode libraries were not found. You may want to try -DENABLE_INTERNAL_LIBQRENCODE=ON")
    endif()
  endif()
endif()
