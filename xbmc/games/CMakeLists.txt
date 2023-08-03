set(SOURCES GameServices.cpp
            GameSettings.cpp
            GameUtils.cpp)

set(HEADERS GameServices.h
            GameSettings.h
            GameTypes.h
            GameUtils.h)

core_add_library(games)

if(ENABLE_STATIC_LIBS)
  target_link_libraries(games PRIVATE Libjuice::Libjuice)
  target_link_libraries(games PRIVATE libsrtp::libsrtp)
  target_link_libraries(games PRIVATE Usrsctp::Usrsctp)
#  target_link_libraries(games PRIVATE LibDataChannel::LibDataChannel)
endif()
