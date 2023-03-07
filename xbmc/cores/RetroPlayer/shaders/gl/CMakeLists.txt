set(SOURCES ShaderGL.cpp
            ShaderLutGL.cpp
            ShaderPresetGL.cpp
            ShaderTextureGLES.cpp
            ShaderUtilsGL.cpp
)

set(HEADERS ShaderGL.h
            ShaderLutGL.h
            ShaderPresetGL.h
            ShaderTextureGLES.h
            ShaderTypesGL.h
            ShaderUtilsGL.h
)

if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES ShaderTextureGL.cpp)
  list(APPEND HEADERS ShaderTextureGL.h)
endif()

core_add_library(rp-shaders-gl)
