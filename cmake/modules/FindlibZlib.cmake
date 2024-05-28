# FindlibZlib
# -----------
# Finds the zlib library
#
# We use the odd name of libZlib to allow us to leverage the cmake system
# find module, and package the target into our setup
#
# This will define the following target:
#
#   ${APP_NAME_LC}::libZlib - The zlib library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  if(Zlib_FIND_REQUIRED)
    set(REQ "REQUIRED")
  endif()

  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(ZLIB_ROOT ${DEPENDS_PATH})
  endif()

  # Darwin platforms link against toolchain provided zlib regardless
  # They will fail when searching for static. All other platforms, prefer static
  # if possible (requires cmake 3.24+ otherwise variable is a no-op)
  # Windows still uses dynamic lib for zlib for other purposes, dont mix
  if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT (WIN32 OR WINDOWS_STORE))
    set(ZLIB_USE_STATIC_LIBS ON)
  endif()
  find_package(ZLIB ${REQ})
  unset(ZLIB_USE_STATIC_LIBS)

  if(ZLIB_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ZLIB::ZLIB)
  else()
    if(Zlib_FIND_REQUIRED)
      message(FATAL_ERROR "Zlib libraries were not found.")
    endif()
  endif()
endif()
