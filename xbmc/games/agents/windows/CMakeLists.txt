set(SOURCES GUIAgentList.cpp
            GUIAgentProfile.cpp
            GUIAgentWindow.cpp
)

set(HEADERS GUIAgentDefines.h
            GUIAgentList.h
            GUIAgentProfile.h
            GUIAgentWindow.h
            IAgentList.h
)

core_add_library(games_agents_windows)
