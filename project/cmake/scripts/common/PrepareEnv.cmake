# parse version.txt and libKODI_guilib.h to get the version and API info
include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/Macros.cmake)
core_find_versions()

# in case we need to download something, set KODI_MIRROR to the default if not alread set
if(NOT DEFINED KODI_MIRROR)
  set(KODI_MIRROR "http://mirrors.kodi.tv")
endif()

### copy all the addon binding header files to include/kodi
# make sure include/kodi exists and is empty
set(APP_LIB_DIR ${DEPENDS_PATH}/lib/${APP_NAME_LC})
if(NOT EXISTS "${APP_LIB_DIR}/")
  file(MAKE_DIRECTORY ${APP_LIB_DIR})
endif()

set(APP_INCLUDE_DIR ${DEPENDS_PATH}/include/${APP_NAME_LC})
if(NOT EXISTS "${APP_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${APP_INCLUDE_DIR})
endif()

# we still need XBMC_INCLUDE_DIR and XBMC_LIB_DIR for backwards compatibility to xbmc
set(XBMC_LIB_DIR ${DEPENDS_PATH}/lib/xbmc)
if(NOT EXISTS "${XBMC_LIB_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_LIB_DIR})
endif()
set(XBMC_INCLUDE_DIR ${DEPENDS_PATH}/include/xbmc)
if(NOT EXISTS "${XBMC_INCLUDE_DIR}/")
  file(MAKE_DIRECTORY ${XBMC_INCLUDE_DIR})
endif()

# make sure C++11 is always set
if(NOT WIN32)
  string(REGEX MATCH "-std=(gnu|c)\\+\\+11" cxx11flag "${CMAKE_CXX_FLAGS}")
  if(NOT cxx11flag)
    set(CXX11_SWITCH "-std=c++11")
  endif()
endif()

# generate the proper KodiConfig.cmake file
configure_file(${CORE_SOURCE_DIR}/project/cmake/KodiConfig.cmake.in ${APP_LIB_DIR}/KodiConfig.cmake @ONLY)

# copy cmake helpers to lib/kodi
file(COPY ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddonHelpers.cmake
          ${CORE_SOURCE_DIR}/project/cmake/scripts/common/AddOptions.cmake
     DESTINATION ${APP_LIB_DIR})

# generate XBMCConfig.cmake for backwards compatibility to xbmc
configure_file(${CORE_SOURCE_DIR}/project/cmake/XBMCConfig.cmake.in ${XBMC_LIB_DIR}/XBMCConfig.cmake @ONLY)

### copy all the addon binding header files to include/kodi
# parse addon-bindings.mk to get the list of header files to copy
file(STRINGS ${CORE_SOURCE_DIR}/xbmc/addons/addon-bindings.mk bindings)
string(REPLACE "\n" ";" bindings "${bindings}")
foreach(binding ${bindings})
  string(REPLACE " =" ";" binding "${binding}")
  string(REPLACE "+=" ";" binding "${binding}")
  list(GET binding 1 header)
  # copy the header file to include/kodi
  file(COPY ${CORE_SOURCE_DIR}/${header} DESTINATION ${APP_INCLUDE_DIR})

  # auto-generate header files for backwards compatibility to xbmc with deprecation warning
  # but only do it if the file doesn't already exist
  get_filename_component(headerfile ${header} NAME)
  if(NOT EXISTS "${XBMC_INCLUDE_DIR}/${headerfile}")
    file(WRITE ${XBMC_INCLUDE_DIR}/${headerfile}
"#pragma once
#define DEPRECATION_WARNING \"Including xbmc/${headerfile} has been deprecated, please use kodi/${headerfile}\"
#ifdef _MSC_VER
  #pragma message(\"WARNING: \" DEPRECATION_WARNING)
#else
  #warning DEPRECATION_WARNING
#endif
#include \"kodi/${headerfile}\"")
  endif()
endforeach()

### processing additional tools required by the platform
if(EXISTS ${APP_ROOT}/project/cmake/scripts/${CORE_SYSTEM_NAME}/tools/)
  file(GLOB platform_tools ${APP_ROOT}/project/cmake/scripts/${CORE_SYSTEM_NAME}/tools/*.cmake)
  foreach(platform_tool ${platform_tools})
    get_filename_component(platform_tool_name ${platform_tool} NAME_WE)
    message(STATUS "Processing ${CORE_SYSTEM_NAME} specific tool: ${platform_tool_name}")

    # include the file
    include(${platform_tool})
  endforeach()
endif()
