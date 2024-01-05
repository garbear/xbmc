set(SOURCES DialogSelectAvatar.cpp
            GUIAvatarDialog.cpp
            GUIAvatarList.cpp
            GUIDialogAgentController.cpp
)

set(HEADERS DialogSelectAvatar.h
            GUIAvatarDefines.h
            GUIAvatarDialog.h
            GUIAvatarList.h
            GUIDialogAgentController.h
            IAvatarList.h
)

core_add_library(games_agents_dialogs)
