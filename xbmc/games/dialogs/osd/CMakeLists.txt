set(SOURCES DialogGameAdvancedSettings.cpp
            DialogGameOSD.cpp
            DialogGameOSDHelp.cpp
            DialogGameSaves.cpp
            DialogGameStretchMode.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoRotation.cpp
            DialogGameVideoSelect.cpp
            DialogGameVolume.cpp
            DialogGameAchievements.cpp
            DialogGameLeaderboards.cpp
            DialogGameLeaderboardEntries.cpp
            DialogInGameSaves.cpp
)

set(HEADERS DialogGameAdvancedSettings.h
            DialogGameOSD.h
            DialogGameOSDHelp.h
            DialogGameSaves.h
            DialogGameStretchMode.h
            DialogGameVideoFilter.h
            DialogGameVideoRotation.h
            DialogGameVideoSelect.h
            DialogGameVolume.h
            DialogGameAchievements.h
            DialogGameLeaderboards.h
            DialogGameLeaderboardEntries.h
            DialogInGameSaves.h
)

core_add_library(gameosddialogs)
