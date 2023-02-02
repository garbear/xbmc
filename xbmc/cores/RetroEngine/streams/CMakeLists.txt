set(SOURCES RetroEngineStreamManager.cpp
            RetroEngineStreamSwFramebuffer.cpp
)

set(HEADERS IRetroEngineStream.h
            RetroEngineStreamManager.h
            RetroEngineStreamSwFramebuffer.h
)

core_add_library(retroengine_streams)
