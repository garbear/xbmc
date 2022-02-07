set(SOURCES DialogGameAchievements.cpp
            DialogGameLeaderboards.cpp
            DialogGameLeaderboardEntries.cpp
            DialogGameAdvancedSettings.cpp
            DialogGameOSD.cpp
            DialogGameSaves.cpp
            DialogGameStretchMode.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoRotation.cpp
            DialogGameVideoSelect.cpp
            DialogGameVolume.cpp
            DialogInGameSaves.cpp
)

set(HEADERS DialogGameAchievements.h
            DialogGameLeaderboards.h
            DialogGameLeaderboardEntries.h
            DialogGameAdvancedSettings.h
            DialogGameOSD.h
            DialogGameSaves.h
            DialogGameStretchMode.h
            DialogGameVideoFilter.h
            DialogGameVideoRotation.h
            DialogGameVideoSelect.h
            DialogGameVolume.h
            DialogInGameSaves.h
)

core_add_library(gameosddialogs)
