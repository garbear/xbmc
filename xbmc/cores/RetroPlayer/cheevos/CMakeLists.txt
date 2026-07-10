set(SOURCES Cheevos.cpp)

set(HEADERS Cheevos.h
            RConsoleIDs.h)

core_add_library(retroplayer_cheevos)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../rcheevos
                 ${CMAKE_CURRENT_BINARY_DIR}/rcheevos)

target_include_directories(retroplayer_cheevos PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../rcheevos/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../rcheevos/src
)

target_link_libraries(retroplayer_cheevos PRIVATE rcheevos)
