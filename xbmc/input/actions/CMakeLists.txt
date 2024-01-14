set(SOURCES Action.cpp
            ActionTranslator.cpp
)

set(HEADERS Action.h
            ActionIDs.h
            ActionTranslator.h
            interfaces/IActionListener.h
)

core_add_library(input_actions)
