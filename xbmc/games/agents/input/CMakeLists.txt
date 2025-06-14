set(SOURCES AgentController.cpp
            AgentInput.cpp
            AgentInputMap.cpp
            AgentInputMapXML.cpp
            AgentJoystick.cpp
            AgentKeyboard.cpp
            AgentMouse.cpp
            AgentTopology.cpp
            AgentTopologyXML.cpp
)

set(HEADERS AgentController.h
            AgentInput.h
            AgentInputMap.h
            AgentInputMapXML.h
            AgentJoystick.h
            AgentKeyboard.h
            AgentMouse.h
            AgentTopology.h
            AgentTopologyXML.h
)

core_add_library(games_agents_input)
