if(OPENGL_FOUND)
  set(SOURCES OSXGLView.mm
              WindowControllerMacOS.mm
              WinSystemOSXGL.mm)
  set(HEADERS OSXGLView.h
              WindowControllerMacOS.h
              WinSystemOSXGL.h)

  core_add_library(windowing_osx_opengl)

endif()
