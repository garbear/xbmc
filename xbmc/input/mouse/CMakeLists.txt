set(SOURCES MouseStat.cpp
            MouseTranslator.cpp
)

set(HEADERS interfaces/IMouseDriverHandler.h
            interfaces/IMouseInputHandler.h
            interfaces/IMouseInputProvider.h
            MouseEvent.h
            MouseStat.h
            MouseTranslator.h
            MouseTypes.h
)

core_add_library(input_mouse)
