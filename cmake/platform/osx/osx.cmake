list(APPEND CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/osx/XBMCApplication.mm)

if(NOT APP_RENDER_SYSTEM OR APP_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
  set(APP_RENDER_SYSTEM gl)
  list(APPEND SYSTEM_DEFINES -DGL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
                             -DGL_SILENCE_DEPRECATION)
else()
  message(SEND_ERROR "Currently only OpenGL rendering is supported. Please set APP_RENDER_SYSTEM to \"gl\"")
endif()

