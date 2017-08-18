set(SOURCES VideoShaderPresetFactory.cpp
            VideoShaderUtils.cpp
)

set(HEADERS IVideoShader.h
            IVideoShaderPreset.h
            IVideoShaderTexture.h
            VideoShaderPresetFactory.h
            VideoShaderUtils.h
)

core_add_library(rp-videoshader)
