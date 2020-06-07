################################################################################
# Lightweight modern wrapper around sqlite C API.
#
#   https://github.com/SqliteModernCpp/sqlite_modern_cpp
#   SPDX-License-Identifier: MIT
#
################################################################################

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

find_package(sqlite3 CONFIG REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/SQLiteModernCppTargets.cmake")
check_required_components("SQLiteModernCpp")
