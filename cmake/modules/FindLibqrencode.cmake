#.rst:
# FindLibqrencode
# -------
# Finds the libqrencode library and headers
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::libqrencode - The libqrencode library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildLibqrencode)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-Fix-building-with-CMake-4.0.patch")

    generate_patchcommand("${patches}")

    # Debug postfix only used for windows
    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX "d")
    endif()

    set(CMAKE_ARGS -DWITH_TOOLS=NO)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libqrencode)

  SETUP_BUILD_VARS()

  # TODO: Check for existing libqrencode. If version >= LIBQRENCODE-VERSION file version, dont build
  if(ENABLE_INTERNAL_LIBQRENCODE)
    if(NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      # Build lib
      buildLibqrencode()
    endif()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(${${CMAKE_FIND_PACKAGE_NAME}_MODULE})
  unset(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Libqrencode
                                    REQUIRED_VARS
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR
                                      ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Libqrencode_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_RELEASE}")
    endif()
    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIBRARY_DEBUG}")
    endif()

    set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                          INTERFACE_INCLUDE_DIRECTORIES "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR}")

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
        buildLibqrencode()
        set_target_properties(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Libqrencode_FIND_REQUIRED)
      message(FATAL_ERROR "libqrencode libraries were not found. You may want to try -DENABLE_INTERNAL_LIBQRENCODE=ON")
    endif()
  endif()
endif()
