set(SOURCES DialogGameAdvancedSettings.cpp
            DialogGameDiscManager.cpp
            DialogGameOSD.cpp
            DialogGameOSDHelp.cpp
            DialogGameSaves.cpp
            DialogGameStretchMode.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoRotation.cpp
            DialogGameVideoSelect.cpp
            DialogGameVolume.cpp
            DialogInGameSaves.cpp
            DiscManagerDiscList.cpp
            DiscManagerMenu.cpp
)

set(HEADERS DialogGameAdvancedSettings.h
            DialogGameDiscManager.h
            DialogGameOSD.h
            DialogGameOSDHelp.h
            DialogGameSaves.h
            DialogGameStretchMode.h
            DialogGameVideoFilter.h
            DialogGameVideoRotation.h
            DialogGameVideoSelect.h
            DialogGameVolume.h
            DialogInGameSaves.h
            DiscManagerDiscList.h
            DiscManagerMenu.h
)

core_add_library(gameosddialogs)
