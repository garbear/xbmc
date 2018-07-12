cmake_minimum_required(VERSION 3.1)

# CMP0048 prevents CMake 4.0+ policy errors
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

project(lmdb)

# Disable warnings
add_definitions(-w)

add_library(${PROJECT_NAME}
  libraries/liblmdb/mdb.c
  libraries/liblmdb/midl.c
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION lib
)

install(
  FILES libraries/liblmdb/lmdb.h
  DESTINATION include
)
