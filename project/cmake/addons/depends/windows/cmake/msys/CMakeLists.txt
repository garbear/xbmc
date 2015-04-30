cmake_minimum_required(VERSION 3.1)
project(msys)

set(MINGW_PATH "${CMAKE_INSTALL_PREFIX}/mingw")
set(MSYS_ETC_FSTAB "${MSYS_PATH}/etc/fstab")

# create /etc/fstab
file(WRITE "${MSYS_ETC_FSTAB}" "${MINGW_PATH} /mingw\n")
