set(SOURCES ButtonMapping.cpp
            DriverReceiving.cpp
            FeatureHandling.cpp
            GenericButtonMap.cpp
            InputHandling.cpp)

set(HEADERS ButtonMapping.h
            DriverReceiving.h
            FeatureHandling.h
            GenericButtonMap.h
            InputHandling.h)

core_add_library(input_joystick_generic)
