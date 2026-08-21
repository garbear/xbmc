set(SOURCES BaseRenderBuffer.cpp
            BaseRenderBufferPool.cpp
            RenderBufferManager.cpp
)

set(HEADERS BaseRenderBuffer.h
            BaseRenderBufferPool.h
            IRenderBuffer.h
            IRenderBufferPool.h
            RenderBufferManager.h
)

if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES RenderBufferOpenGL.cpp
                      RenderBufferPoolOpenGL.cpp)
  list(APPEND HEADERS RenderBufferOpenGL.h
                      RenderBufferPoolOpenGL.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES RenderBufferOpenGLES.cpp
                      RenderBufferPoolOpenGLES.cpp)
  list(APPEND HEADERS RenderBufferOpenGLES.h
                      RenderBufferPoolOpenGLES.h)
endif()

if(("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC) AND TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES RenderBufferDMA.cpp
                      RenderBufferPoolDMA.cpp)
  list(APPEND HEADERS RenderBufferDMA.h
                      RenderBufferPoolDMA.h)

  if(TARGET ${APP_NAME_LC}::OpenGl)
    list(APPEND SOURCES RenderBufferDMAOpenGL.cpp
                        RenderBufferPoolDMAOpenGL.cpp)
    list(APPEND HEADERS RenderBufferDMAOpenGL.h
                        RenderBufferPoolDMAOpenGL.h)
  endif()

  if(TARGET ${APP_NAME_LC}::OpenGLES)
    list(APPEND SOURCES RenderBufferDMAOpenGLES.cpp
                        RenderBufferPoolDMAOpenGLES.cpp)
    list(APPEND HEADERS RenderBufferDMAOpenGLES.h
                        RenderBufferPoolDMAOpenGLES.h)
  endif()
endif()

# The FBO pool creates a shared EGL context off the windowing system, so it
# requires EGL in addition to desktop GL -- but nothing from DMA, and no
# particular windowing system. Desktop GL and GLES are both served: the pool
# asks EGL for whichever this build renders with.
if((TARGET ${APP_NAME_LC}::OpenGl OR TARGET ${APP_NAME_LC}::OpenGLES) AND TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES RenderBufferFBO.cpp
                      RenderBufferPoolFBO.cpp)
  list(APPEND HEADERS RenderBufferFBO.h
                      RenderBufferPoolFBO.h)
endif()

core_add_library(rp-buffers)
