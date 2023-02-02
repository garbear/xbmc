set(SOURCES GUIRERenderHandle.cpp
            GUIRERenderSettings.cpp
            GUIRERenderTarget.cpp
            GUIRERenderTargetFactory.cpp
            RetroEngineGuiBridge.cpp
            RetroEngineGuiManager.cpp
)

set(HEADERS GUIRERenderHandle.h
            GUIRERenderSettings.h
            GUIRERenderTarget.h
            GUIRERenderTargetFactory.h
            RetroEngineGuiBridge.h
            RetroEngineGuiManager.h
)

core_add_library(retroengine_guibridge)
