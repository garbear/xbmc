set(SOURCES RPProcessInfo.cpp
)

set(HEADERS RPProcessInfo.h
)

if(EGL_FOUND)
  list(APPEND SOURCES egl/RPProcessInfoEGL.cpp)
  list(APPEND HEADERS egl/RPProcessInfoEGL.h)
endif()

core_add_library(rp-process)
