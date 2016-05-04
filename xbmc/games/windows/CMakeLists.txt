set(SOURCES GUIViewStateWindowGames.cpp
            GUIWindowGames.cpp)

set(HEADERS GUIViewStateWindowGames.h
            GUIWindowGames.h)

core_add_library(gameswindows)
add_dependencies(gameswindows libcpluff)
