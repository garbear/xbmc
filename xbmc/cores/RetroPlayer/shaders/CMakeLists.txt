set(SOURCES ShaderArtifactStore.cpp
            ShaderCompileService.cpp
            ShaderPreset.cpp
            ShaderPresetFactory.cpp
            ShaderUtils.cpp
)

set(HEADERS IShader.h
            IShaderCompiler.h
            IShaderLut.h
            IShaderPreset.h
            IShaderPresetLoader.h
            IShaderSampler.h
            IShaderTexture.h
            ShaderArtifactStore.h
            ShaderCompileService.h
            ShaderCompileTypes.h
            ShaderPreset.h
            ShaderPresetFactory.h
            ShaderTypes.h
            ShaderUtils.h
)

core_add_library(rp-shaders)
