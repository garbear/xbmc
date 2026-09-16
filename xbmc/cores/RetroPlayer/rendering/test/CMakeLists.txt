set(SOURCES TestBaseRenderBufferPool.cpp
            TestHwRenderingContextEGL.cpp
            TestRPRenderManager.cpp
            TestRenderBufferPoolFBO.cpp
            TestRenderGeometryFBO.cpp)

if(CORE_SYSTEM_NAME STREQUAL "osx")
  list(APPEND SOURCES TestHwRenderingContextOSX.mm)
endif()

core_add_test_library(retroplayer_rendering_test)
