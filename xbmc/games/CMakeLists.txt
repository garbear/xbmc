set(SOURCES GameManager.cpp
            GameSettings.cpp)

set(HEADERS GameManager.h
            GameSettings.h
            GameTypes.h)

core_add_library(games)
add_dependencies(games libcpluff)
