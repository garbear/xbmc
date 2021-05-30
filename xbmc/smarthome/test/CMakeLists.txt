set(SOURCES TestImageSubscriptionKey.cpp
)

if(ROS2_FOUND)
  list(APPEND SOURCES TestRos2PeripheralBattery.cpp
                      TestRos2PeripheralSubscriber.cpp)
endif()

core_add_test_library(smarthome_test)

if(ROS2_FOUND AND TARGET smarthome_test)
  target_link_libraries(smarthome_test PRIVATE smarthome_ros2)
endif()
