set(SOURCES Ros2.cpp
            Ros2InputPublisher.cpp
            Ros2Node.cpp
            Ros2Translator.cpp
            Ros2VideoSubscription.cpp
)
set(HEADERS Ros2.h
            Ros2InputPublisher.h
            Ros2Node.h
            Ros2Translator.h
            Ros2VideoSubscription.h
)

core_add_library(smarthome_ros2)

target_link_libraries(smarthome_ros2
  image_transport::image_transport
)
