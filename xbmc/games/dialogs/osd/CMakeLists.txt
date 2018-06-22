set(SOURCES DialogGameAdvancedSettings.cpp
            DialogGameOSD.cpp
            DialogGameRewind.cpp
            DialogGameSaves.cpp
            DialogGameStretchMode.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoRotation.cpp
            DialogGameVideoSelect.cpp
            DialogGameVolume.cpp
            DialogInGameSaves.cpp
)

set(HEADERS DialogGameAdvancedSettings.h
            DialogGameOSD.h
            DialogGameRewind.h
            DialogGameSaves.h
            DialogGameStretchMode.h
            DialogGameVideoFilter.h
            DialogGameVideoRotation.h
            DialogGameVideoSelect.h
            DialogGameVolume.h
            DialogInGameSaves.h
)

core_add_library(gameosddialogs)
