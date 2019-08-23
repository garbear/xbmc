set(SOURCES ShaderGL.cpp
            ShaderLutGL.cpp
            ShaderPresetGL.cpp
            ShaderTextureGL.cpp
            ShaderUtilsGL.cpp
)

set(HEADERS ShaderGL.h
            ShaderLutGL.h
            ShaderPresetGL.h
            ShaderTextureGL.h
            ShaderTypesGL.h
            ShaderUtilsGL.h
)

core_add_library(rp-shaders-gl)
