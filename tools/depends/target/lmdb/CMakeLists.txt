cmake_minimum_required(VERSION 3.5...4.0)

project(lmdb LANGUAGES C)

add_library(${PROJECT_NAME} STATIC
  libraries/liblmdb/mdb.c
  libraries/liblmdb/midl.c
)

if(MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
  )
endif()

if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX d)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
  target_compile_definitions(${PROJECT_NAME} PRIVATE MDB_USE_UWP)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/liblmdb
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION lib
)

install(
  FILES
    libraries/liblmdb/lmdb.h
    libraries/liblmdb/midl.h
  DESTINATION include
)
