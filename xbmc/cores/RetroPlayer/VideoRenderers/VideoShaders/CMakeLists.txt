set(SOURCES RPConvolutionKernels.cpp)

set(HEADERS RPConvolutionKernels.h
            dither.h
            ShaderFormats.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES RPWinVideoFilter.cpp
                      RPYUV2RGBShader.cpp)
  list(APPEND HEADERS RPWinVideoFilter.h
                      RPYUV2RGBShader.h)
endif()

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES RPGLSLOutput.cpp
                      RPVideoFilterShader.cpp
                      RPYUV2RGBShader.cpp)
  list(APPEND HEADERS RPGLSLOutput.h
                      RPVideoFilterShader.h
                      RPYUV2RGBShader.h)
endif()

core_add_library(rp_videoshaders)
