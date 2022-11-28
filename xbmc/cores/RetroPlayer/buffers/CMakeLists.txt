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
  list(APPEND SOURCES RenderBufferFBO.cpp
                      RenderBufferOpenGL.cpp
                      RenderBufferPoolFBO.cpp
                      RenderBufferPoolOpenGL.cpp)
  list(APPEND HEADERS RenderBufferFBO.h
                      RenderBufferOpenGL.h
                      RenderBufferPoolFBO.h
                      RenderBufferPoolOpenGL.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES RenderBufferFBO.cpp
                      RenderBufferOpenGLES.cpp
                      RenderBufferPoolFBO.cpp
                      RenderBufferPoolOpenGLES.cpp)
  list(APPEND HEADERS RenderBufferFBO.h
                      RenderBufferOpenGLES.h
                      RenderBufferPoolFBO.h
                      RenderBufferPoolOpenGLES.h)
endif()

if(("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC) AND TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES RenderBufferDMA.cpp
                      RenderBufferPoolDMA.cpp)
  list(APPEND HEADERS RenderBufferDMA.h
                      RenderBufferPoolDMA.h)
endif()

core_add_library(rp-buffers)
