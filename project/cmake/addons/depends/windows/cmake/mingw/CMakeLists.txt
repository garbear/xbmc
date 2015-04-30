cmake_minimum_required(VERSION 3.1)
project(mingw)

function(generate_mingw32_wrapper cmd)
  set(CMD ${cmd})
  configure_file(${PROJECT_SOURCE_DIR}/mingw32-cmd.bat.in ${MINGW_PATH}/bin/${CMD}.bat @ONLY)
endfunction()

set(MSYS_PATH "${CMAKE_INSTALL_PREFIX}/msys")
set(MINGW_PATH "${CMAKE_INSTALL_PREFIX}/mingw")

# configure the MinGW toolchain file
configure_file(${PROJECT_SOURCE_DIR}/Toolchain_mingw32.cmake.in ${CMAKE_INSTALL_PREFIX}/Toolchain_mingw32.cmake @ONLY)

# configure MinGWConfig.cmake
configure_file(${PROJECT_SOURCE_DIR}/MinGWConfig.cmake.in ${CMAKE_INSTALL_PREFIX}/MinGWConfig.cmake)

# configure the MinGW wrapper batch scripts
generate_mingw32_wrapper("mingw32-make")
generate_mingw32_wrapper("mingw32-gcc")
generate_mingw32_wrapper("mingw32-g++")
generate_mingw32_wrapper("ar")
generate_mingw32_wrapper("ld")
generate_mingw32_wrapper("windres")
