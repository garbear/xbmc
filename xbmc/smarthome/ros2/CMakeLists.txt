set(SOURCES Ros2.cpp
            Ros2Node.cpp
            Ros2Translator.cpp
            Ros2VideoNode.cpp
            Ros2VideoSubscription.cpp
)
set(HEADERS Ros2.h
            Ros2Node.h
            Ros2Translator.h
            Ros2VideoNode.h
            Ros2VideoSubscription.h
)

core_add_library(smarthome_ros2)
