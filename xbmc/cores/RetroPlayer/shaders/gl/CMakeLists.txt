set(SOURCES ShaderTextureGL.cpp
        ShaderUtilsGL.cpp
        ShaderPresetGL.cpp
        ShaderLutGL.cpp
        ShaderGL.cpp)

set(HEADERS ShaderTextureGL.h
        ShaderUtilsGL.h
        ShaderLutGL.h
        ShaderGL.h
        ShaderTypesGL.h)

core_add_library(rp-shaders-gl)
