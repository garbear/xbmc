# if(CORE_SYSTEM_NAME STREQUAL windows)
#   // list(APPEND SOURCES RPDXVAHD.cpp)
#   // list(APPEND HEADERS RPDXVAHD.h)
# endif()

if(CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND SOURCES RPRendererVTBGL.cpp)
  list(APPEND HEADERS RPRendererVTBGL.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  list(APPEND SOURCES RPRendererVTBGLES.cpp)
  list(APPEND HEADERS RPRendererVTBGLES.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES RPMMALRenderer.cpp)
  list(APPEND HEADERS RPMMALRenderer.h)
endif()

if(AML_FOUND)
  list(APPEND SOURCES RPRendererAML.cpp)
  list(APPEND HEADERS RPRendererAML.h)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES RPRendererIMX.cpp)
  list(APPEND HEADERS RPRendererIMX.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES RPRendererMediaCodec.cpp
                      RPRendererMediaCodecSurface.cpp)
  list(APPEND HEADERS RPRendererMediaCodec.h
                      RPRendererMediaCodecSurface.h)
endif()

# we might want to build on linux systems
# with ENABLE_VDPAU=OFF and ENABLE_VAAPI=OFF
if(SOURCES)
  core_add_library(rp_videorenderers_hwdec)
endif()
