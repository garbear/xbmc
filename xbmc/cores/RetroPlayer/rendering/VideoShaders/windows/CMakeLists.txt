set(SOURCES RPWinOutputShader.cpp
            VideoShaderDX.cpp
            VideoShaderPresetDX.cpp
            VideoShaderTextureDX.cpp)

set(HEADERS RPWinOutputShader.h
            VideoShaderDX.h
            VideoShaderTextureDX.h
            VideoShaderPresetDX.h
            VideoShaderUtilsDX.h)

core_add_library(rp-videoshader-windows)
