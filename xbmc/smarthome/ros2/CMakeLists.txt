set(HEADERS IRos2.h)

if (ROS2_FOUND)
  list(APPEND SOURCES Ros2.cpp
                      Ros2InputPublisher.cpp
                      Ros2Node.cpp
                      Ros2NowPlayingPublisher.cpp
                      Ros2PeripheralSubscriber.cpp
                      Ros2PowerMeterManager.cpp
                      Ros2PowerMeterSubscriber.cpp
                      Ros2PropertyManager.cpp
                      Ros2PropertySubscriber.cpp
                      Ros2SystemHealthManager.cpp
                      Ros2SystemHealthSubscriber.cpp
                      Ros2Translator.cpp
                      Ros2VehicleManager.cpp
                      Ros2VehicleSubscriber.cpp
                      Ros2VideoSubscription.cpp
  )
  list(APPEND HEADERS Ros2.h
                      Ros2InputPublisher.h
                      Ros2Node.h
                      Ros2NowPlayingPublisher.h
                      Ros2PeripheralSubscriber.h
                      Ros2PowerMeterManager.h
                      Ros2PowerMeterSubscriber.h
                      Ros2PropertyManager.h
                      Ros2PropertySubscriber.h
                      Ros2SystemHealthManager.h
                      Ros2SystemHealthSubscriber.h
                      Ros2Translator.h
                      Ros2VehicleManager.h
                      Ros2VehicleSubscriber.h
                      Ros2VideoSubscription.h
  )

  core_add_library(smarthome_ros2)

  target_link_libraries(${CORE_LIBRARY} PUBLIC
    image_transport::image_transport
    ${oasis_msgs_TARGETS}
  )
else()
  if (NOT ENABLE_STATIC_LIBS)
    core_add_library(smarthome_ros2)
  endif()
endif()
