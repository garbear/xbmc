set(SOURCES SmartHomeStreams.cpp
            SmartHomeStreamSwFramebuffer.cpp
)

set(HEADERS SmartHomeStreams.h
            SmartHomeStreamSwFramebuffer.h
            ISmartHomeStream.h
)

core_add_library(smarthome_streams)
