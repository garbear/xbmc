################################################################################
# Lightweight modern wrapper around sqlite C API.
#
#   https://github.com/SqliteModernCpp/sqlite_modern_cpp
#   SPDX-License-Identifier: MIT
#
################################################################################

# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

# Create imported target SQLiteModernCpp::SQLiteModernCpp
add_library(SQLiteModernCpp::SQLiteModernCpp INTERFACE IMPORTED)

set_target_properties(SQLiteModernCpp::SQLiteModernCpp PROPERTIES
  INTERFACE_COMPILE_FEATURES "cxx_std_17"
  INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include"
  INTERFACE_LINK_LIBRARIES "sqlite3::sqlite3"
)

# Cleanup temporary variables.
set(_IMPORT_PREFIX)
