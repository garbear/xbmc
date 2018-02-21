set(SOURCES RenderContext.cpp
            RenderResourceSysMem.cpp
            RenderSettings.cpp
            RenderTranslator.cpp
            RenderUtils.cpp
            RenderVideoSettings.cpp
            RPRenderManager.cpp
)

set(HEADERS IRenderManager.h
            IRenderResource.h
            IRenderResourceMemory.h
            RenderContext.h
            RenderResourceSysMem.h
            RenderSettings.h
            RenderTranslator.h
            RenderUtils.h
            RenderVideoSettings.h
            RPRenderManager.h
)

core_add_library(rp-rendering)
