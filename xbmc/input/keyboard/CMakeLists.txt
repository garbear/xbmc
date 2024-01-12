set(SOURCES KeyboardEasterEgg.cpp
            KeyboardTranslator.cpp
            KeymapActionMap.cpp
)

set(HEADERS interfaces/IActionMap.h
            interfaces/IKeyboardDriverHandler.h
            interfaces/IKeyboardInputHandler.h
            interfaces/IKeyboardInputProvider.h
            KeyboardEasterEgg.h
            KeyboardSymbols.h
            KeyboardTranslator.h
            KeyboardTypes.h
            KeymapActionMap.h
)

core_add_library(input_keyboard)
