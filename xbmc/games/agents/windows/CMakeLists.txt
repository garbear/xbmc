set(SOURCES GUIAgentAvatarList.cpp
            GUIAgentControllerList.cpp
            GUIAgentWindow.cpp
)

set(HEADERS GUIAgentAvatarList.h
            GUIAgentControllerList.h
            GUIAgentDefines.h
            GUIAgentWindow.h
            IAgentAvatarList.h
            IAgentControllerList.h
)

core_add_library(games_agents_windows)
