# FindGBM
# ----------
# Finds the GBM library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::GBM   - The GBM library
#
# It will also define:
#
#   GBM_FOUND             - TRUE if GBM was found
#
# Notes:
# - This module must either create the target when found, or fail cleanly
#   when REQUIRED is used, instead of letting generation fail later.

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC gbm)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_DISABLE_VERSION ON)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  set(_gbm_found FALSE)

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)

    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    check_c_source_compiles("#include <gbm.h>

                             int main()
                             {
                               gbm_bo_map(NULL, 0, 0, 0, 0, GBM_BO_TRANSFER_WRITE, NULL, NULL);
                             }
                             " GBM_HAS_BO_MAP)

    check_c_source_compiles("#include <gbm.h>

                             int main()
                             {
                               gbm_surface_create_with_modifiers(NULL, 0, 0, 0, NULL, 0);
                             }
                             " GBM_HAS_MODIFIERS)

    unset(CMAKE_REQUIRED_LIBRARIES)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_GBM)

    if(GBM_HAS_BO_MAP)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_GBM_BO_MAP)
    endif()
    if(GBM_HAS_MODIFIERS)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_GBM_MODIFIERS)
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    set(_gbm_found TRUE)
  endif()

  # Proper REQUIRED / QUIET behavior for find_package(GBM [REQUIRED])
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
    REQUIRED_VARS _gbm_found
  )

  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND ${_gbm_found})
  unset(_gbm_found)

endif()
