# Copyright (C) 2026 Team Kodi
# This file is part of Kodi - https://kodi.tv
#
# SPDX-License-Identifier: GPL-2.0-or-later
# See LICENSES/README.md for more information.

cmake_minimum_required(VERSION 3.20)
project(ROS2Probe LANGUAGES C CXX)

get_filename_component(kodi_modules "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
list(PREPEND CMAKE_MODULE_PATH "${kodi_modules}")
find_package(ROS2 REQUIRED MODULE)
