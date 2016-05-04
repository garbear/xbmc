set(SOURCES libKODI_game.cpp)

core_add_library(KODI_game NO_MAIN_DEPENDS)
set_target_properties(KODI_game PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(KODI_game PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi
                           ${CORE_SOURCE_DIR}/xbmc)
