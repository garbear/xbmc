cmake_minimum_required(VERSION 3.0)

project(QrCodeGen VERSION 1.6.0)

# Dependencies
include(CMakePackageConfigHelpers)

# Source parameters
set(SOURCES cpp/QrCode.cpp)
set(HEADERS cpp/QrCode.hpp)

set(TARGET_NAME qrcodegen)

# Configuration parameters
set(NAMESPACE "${PROJECT_NAME}::")

set(CONFIG_INSTALL_DIR "lib/cmake/${PROJECT_NAME}")
set(INCLUDE_INSTALL_DIR "include")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")

# Generated files
write_basic_package_version_file(
  "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  "Config.cmake.in"
  "${PROJECT_CONFIG}"
  INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

# Build targets
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

# Install targets
install(
  TARGETS ${TARGET_NAME}
  EXPORT "${TARGETS_EXPORT_NAME}"
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib"
  INCLUDES DESTINATION "${INCLUDE_INSTALL_DIR}"
)
install(
  FILES ${HEADERS}
  DESTINATION "${INCLUDE_INSTALL_DIR}"
)
install(
  FILES "${PROJECT_CONFIG}" "${VERSION_CONFIG}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)
install(
  EXPORT "${TARGETS_EXPORT_NAME}"
  NAMESPACE "${NAMESPACE}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)
