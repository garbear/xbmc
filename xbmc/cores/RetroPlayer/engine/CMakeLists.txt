set(SOURCES Action.cpp
            GeneralEmulationEngine.cpp
            Policy.cpp
            Reward.cpp
            SpecialEmulationEngine.cpp
            State.cpp
)

set(HEADERS Action.h
            GeneralEmulationEngine.h
            IEmulationEngine.h
            Policy.h
            Reward.h
            SpecialEmulationEngine.h
            State.h
)

core_add_library(retroplayer_engine)

set(DEPENDS retroplayer_messages)

if(ENABLE_STATIC_LIBS)
  add_dependencies(retroplayer_engine ${DEPENDS})
else()
  add_dependencies(lib${APP_NAME_LC} ${DEPENDS})
endif()
