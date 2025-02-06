#if(TARGET ${APP_NAME_LC}::OpenGl)
if(TARGET OpenGL::GL)
  list(APPEND SOURCES ShaderGL.cpp
                      ShaderLutGL.cpp
                      ShaderPresetGL.cpp
                      ShaderTextureGL.cpp
                      ShaderUtilsGL.cpp
  )
  list(APPEND HEADERS ShaderGL.h
                      ShaderLutGL.h
                      ShaderPresetGL.h
                      ShaderTextureGL.h
                      ShaderUtilsGL.h
  )

  core_add_library(rp-shaders-gl)
endif()
