set(SOURCES GameClientBasicPlayback.cpp
            GameClientReversiblePlayback.cpp
            GameLoop.cpp)

set(HEADERS GameClientBasicPlayback.h
            GameClientReversiblePlayback.h
            GameLoop.h
            IGameClientPlayback.h)

core_add_library(gameplayback)
add_dependencies(gameplayback libcpluff)
