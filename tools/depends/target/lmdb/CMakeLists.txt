cmake_minimum_required(VERSION 3.5...4.0)

project(lmdb LANGUAGES C)

add_library(${PROJECT_NAME}
  libraries/liblmdb/mdb.c
  libraries/liblmdb/midl.c
)

if(MSVC)
  # Silence "unsafe" CRT deprecation warnings (C4996) for LMDB code
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
  )
endif()

if(WINDOWS_STORE OR CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    WINAPI_FAMILY=WINAPI_FAMILY_APP
  )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/liblmdb
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION lib
)

install(
  FILES libraries/liblmdb/lmdb.h
  DESTINATION include
)
