#.rst:
# FindInternalBoost
# -------
# Finds the required boost libraries
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::InternalBoost - The boost library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  macro(buildBoost)
    set(BOOST_VERSION ${${MODULE}_VER})

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Add-libraries-to-excludes-list.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                   # context fails on macOS x64
                   # cobalt fails on UWP x86/x64
                   # stacktrace fails on UWP ARM
                   -DBUILD_SHARED_LIBS=OFF)

    BUILD_DEP_TARGET()
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC boost)

  SETUP_BUILD_VARS()

  # TODO: Check for existing boost. If version >= BOOST-VERSION file version, dont build
  if(ENABLE_INTERNAL_BOOST)
    # Build lib
    buildBoost()
  else()
    # TODO
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(BOOST)
  unset(BOOST_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(InternalBoost
                                    REQUIRED_VARS BOOST_LIBRARY BOOST_INCLUDE_DIR
                                    VERSION_VAR BOOST_VERSION)

  if(InternalBoost_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    if(BOOST_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${BOOST_LIBRARY_RELEASE}")
    endif()
    if(BOOST_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS DEBUG
                                                                       IMPORTED_LOCATION_DEBUG "${BOOST_LIBRARY_DEBUG}")
    endif()
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     # TODO
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${BOOST_INCLUDE_DIR};${BOOST_INCLUDE_DIR}/boost-1_85")

    if(TARGET boost)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} boost)
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
      if(NOT TARGET boost)
        buildBoost()
        set_target_properties(boost PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends boost)
    endif()
  else()
    if(BOOST_FIND_REQUIRED)
      message(FATAL_ERROR "boost libraries were not found. You may want to try -DENABLE_INTERNAL_BOOST=ON")
    endif()
  endif()
endif()
