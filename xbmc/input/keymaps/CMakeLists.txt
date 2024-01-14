set(SOURCES AppTranslator.cpp
            ButtonStat.cpp
            ButtonTranslator.cpp
            Keymap.cpp
            KeymapEnvironment.cpp
            WindowKeymap.cpp
)

set(HEADERS interfaces/IKeyMapper.h
            interfaces/IKeyHandler.h
            interfaces/IKeyboardActionMap.h
            interfaces/IKeymapHandler.h
            interfaces/IKeymapEnvironment.h
            interfaces/IKeymap.h
            AppTranslator.h
            ButtonStat.h
            ButtonTranslator.h
            Keymap.h
            KeymapEnvironment.h
            KeymapTypes.h
            WindowKeymap.h
)

core_add_library(input_keymaps)
