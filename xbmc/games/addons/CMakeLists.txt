set(SOURCES GameClient.cpp
            GameClientInput.cpp
            GameClientProperties.cpp
            GameClientTiming.cpp
            GameClientTranslator.cpp)

set(HEADERS GameClient.h
            GameClientCallbacks.h
            GameClientInput.h
            GameClientProperties.h
            GameClientTiming.h
            GameClientTranslator.h)

core_add_library(gameaddons)
add_dependencies(gameaddons libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(gameaddons ffmpeg)
endif()
