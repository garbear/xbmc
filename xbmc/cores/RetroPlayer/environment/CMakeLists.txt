set(SOURCES Action.cpp
            ActionController.cpp
            ActionKeyboard.cpp
            ActionTopology.cpp
            Environment.cpp
            State.cpp
)

set(HEADERS Action.h
            ActionController.h
            ActionKeyboard.h
            ActionTopology.h
            Environment.h
            State.h
)

core_add_library(retroplayer_environment)
