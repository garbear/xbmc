# Copyright (C) 2026 Team Kodi
# This file is part of Kodi - https://kodi.tv
#
# SPDX-License-Identifier: GPL-2.0-or-later
# See LICENSES/README.md for more information.

set(ROS2_FOUND FALSE)
if(NOT ROS2_FIND_REQUIRED)
  # Ament configs can fail fatally while importing a transitive dependency
  set(probe_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/ROS2Probe")
  file(MAKE_DIRECTORY "${probe_dir}")
  file(REMOVE "${probe_dir}/CMakeCache.txt")

  # Carry toolchain and package search settings into the isolated configure
  set(probe_cache "")
  get_cmake_property(settings CACHE_VARIABLES)
  list(APPEND settings CMAKE_PREFIX_PATH CMAKE_MODULE_PATH CMAKE_FIND_ROOT_PATH
                       CMAKE_IGNORE_PREFIX_PATH)
  list(REMOVE_DUPLICATES settings)
  foreach(var IN LISTS settings)
    get_property(type CACHE "${var}" PROPERTY TYPE)
    if(NOT type MATCHES "^(INTERNAL|STATIC)$" AND
       (var MATCHES "^CMAKE_|(_DIR|_ROOT|_EXECUTABLE)$" OR type STREQUAL "UNINITIALIZED"))
      set(delimiter "=")
      while("${var}];${${var}}]" MATCHES "]${delimiter}]")
        string(APPEND delimiter "=")
      endwhile()
      string(APPEND probe_cache
        "set([${delimiter}[${var}]${delimiter}] [${delimiter}[${${var}}]${delimiter}] CACHE STRING \"\")\n")
    endif()
  endforeach()
  file(WRITE "${probe_dir}/settings.cmake" "${probe_cache}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/ros2"
                          -B "${probe_dir}" -G "${CMAKE_GENERATOR}"
                          -A "${CMAKE_GENERATOR_PLATFORM}" -T "${CMAKE_GENERATOR_TOOLSET}"
                          "-DCMAKE_GENERATOR_INSTANCE=${CMAKE_GENERATOR_INSTANCE}"
                          -C "${probe_dir}/settings.cmake"
                  RESULT_VARIABLE probe_result OUTPUT_QUIET
                  ERROR_FILE "${probe_dir}/configure.log")
  if(NOT probe_result EQUAL 0)
    message(STATUS "Disabling ROS 2 support (see ${probe_dir}/configure.log)")
    return()
  endif()
endif()

# ROS package configs must load the same dependencies in the probe and in Kodi
set(ros2_module_path "${CMAKE_MODULE_PATH}")
list(REMOVE_ITEM CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/buildtools")
foreach(dep IN ITEMS ament_cmake builtin_interfaces geometry_msgs image_transport
                     oasis_msgs rcl_interfaces rclcpp rmw sensor_msgs std_msgs)
  find_package(${dep} REQUIRED CONFIG)
endforeach()
set(CMAKE_MODULE_PATH "${ros2_module_path}")
set(ROS2_FOUND TRUE)
