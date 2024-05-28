# FindSSL
# -----------
# Finds the Openssl libraries
#
# This will define the following target:
#
#   ${APP_NAME_LC}::SSL - The OpenSSL libraries

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(SSLFIND_REQUIRED)
    set(REQ "REQUIRED")
  endif()

  # Only aim for static libs on windows or depends builds
  if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
    set(OPENSSL_USE_STATIC_LIBS ON)
    set(OPENSSL_ROOT_DIR ${DEPENDS_PATH})
  endif()
  find_package(OpenSSL ${REQ})
  unset(OPENSSL_USE_STATIC_LIBS)

  if(OPENSSL_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS OpenSSL::SSL)
    add_library(${APP_NAME_LC}::Crypto ALIAS OpenSSL::Crypto)

    # Add Crypto as a link library to easily propagate both targets to our custom target
    set_target_properties(OpenSSL::SSL PROPERTIES
                                       INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::Crypto")
  else()
    if(SSL_FIND_REQUIRED)
      message(FATAL_ERROR "SSL libraries were not found.")
    endif()
  endif()
endif()
