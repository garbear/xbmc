set(SOURCES GUIRenderHandle.cpp
            GUIRenderSettings.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
            RetroEngineGuiBridge.cpp
            RetroEngineGuiManager.cpp
)

set(HEADERS GUIRenderHandle.h
            GUIRenderSettings.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            RetroEngineGuiBridge.h
            RetroEngineGuiManager.h
)

core_add_library(retroengine_guibridge)
