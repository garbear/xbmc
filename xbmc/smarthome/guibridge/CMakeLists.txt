set(SOURCES GUIRenderHandle.cpp
            GUIRenderTarget.cpp
            GUIRenderTargetFactory.cpp
            SmartHomeGuiBridge.cpp
)

set(HEADERS GUIRenderHandle.h
            GUIRenderTarget.h
            GUIRenderTargetFactory.h
            SmartHomeGuiBridge.h
)

core_add_library(smarthome_guibridge)
