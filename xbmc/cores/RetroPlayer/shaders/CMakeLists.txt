set(SOURCES IShaderTexture.cpp
            ShaderPresetFactory.cpp
            ShaderUtils.cpp
)

set(HEADERS IShader.h
            IShaderLut.h
            IShaderPreset.h
            IShaderSampler.h
            IShaderTexture.h
            ShaderPresetFactory.h
            ShaderTypes.h
            ShaderUtils.h
)

core_add_library(rp-shaders)
