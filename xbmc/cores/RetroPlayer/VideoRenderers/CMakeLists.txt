set(SOURCES RPBaseRenderer.cpp
            RPProcessInfo.cpp
            RPRenderFlags.cpp
            RPRenderManager.cpp
            RPRenderUtils.cpp
            RPVideoPicture.cpp)

set(HEADERS RPBaseRenderer.h
            RPProcessInfo.h
            RPRenderFlags.h
            RPRenderFormats.h
            RPRenderManager.h
            RPRenderUtils.h
            RPVideoPicture.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES RPWinRenderer.cpp)
  list(APPEND HEADERS RPWinRenderer.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES RPLinuxRendererGL.cpp
                      RPFrameBufferObject.cpp)
  list(APPEND HEADERS RPLinuxRendererGL.h
                      RPFrameBufferObject.h)
endif()

if(OPENGLES_FOUND)
  list(APPEND SOURCES RPLinuxRendererGLES.cpp
                      RPFrameBufferObject.cpp)
  list(APPEND HEADERS RPLinuxRendererGLES.h
                      RPFrameBufferObject.cpp)
endif()

if(ARCH MATCHES arm)
  list(APPEND SOURCES yuv2rgb.neon.S)
  list(APPEND HEADERS yuv2rgb.neon.h)
endif()

core_add_library(rp_videorenderers)
