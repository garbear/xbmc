#.rst:
# FindXRandR
# ----------
# Finds the XRandR library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::XRandR   - The XRANDR library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC xrandr)

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  set(_xrandr_found FALSE)
  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    set(_xrandr_found TRUE)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_LIBXRANDR)
    ADD_TARGET_COMPILE_DEFINITION()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
    REQUIRED_VARS _xrandr_found
  )

  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND ${_xrandr_found})
  unset(_xrandr_found)

endif()
