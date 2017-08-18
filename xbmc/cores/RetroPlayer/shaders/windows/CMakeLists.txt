set(SOURCES RPWinOutputShader.cpp
            ShaderDX.cpp
            ShaderLutDX.cpp
            ShaderPresetDX.cpp
            ShaderSamplerDX.cpp
            ShaderUtilsDX.cpp)

set(HEADERS RPWinOutputShader.h
            ShaderDX.h
            ShaderLutDX.h
            ShaderPresetDX.h
            ShaderSamplerDX.h
            ShaderTextureDX.h
            ShaderTypesDX.h
            ShaderUtilsDX.h)

core_add_library(rp-shaders-windows)
