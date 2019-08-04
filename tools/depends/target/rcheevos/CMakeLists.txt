project(rcheevos)

cmake_minimum_required(VERSION 3.0)

# Disable Lua for now
add_definitions(-DRC_DISABLE_LUA)

include_directories(
  include
)

add_library(${PROJECT_NAME}lib
  src/rcheevos/alloc.c
  src/rcheevos/condition.c
  src/rcheevos/condset.c
  src/rcheevos/expression.c
  src/rcheevos/format.c
  src/rcheevos/lboard.c
  src/rcheevos/memref.c
  src/rcheevos/operand.c
  src/rcheevos/richpresence.c
  src/rcheevos/term.c
  src/rcheevos/trigger.c
  src/rcheevos/value.c
  src/rurl/url.c
)

#add_dependencies(rcheevos
#  lua # TODO
#)

#target_link_libraries(rcheevos
#  lua
#)

install(TARGETS
	${PROJECT_NAME}lib
  DESTINATION
    lib
)

#install(
#  TARGETS
#    rcheevoslib
#  ARCHIVE DESTINATION
#    ${CMAKE_INSTALL_PREFIX}/lib
#  DESTINATION
#    lib
#)

install(
  FILES
    include/rcheevos.h
    include/rurl.h
  DESTINATION
    include
)

#install(
#  FILES
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/rcheevos.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/rurl.h
#  DESTINATION
#    ${CMAKE_INSTALL_PREFIX}/../include
#  DESTINATION
#    .
#)

#install(
#  DIRECTORY
#    include
#  DESTINATION
#    include
#)
