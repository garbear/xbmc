# FindLMDB
# --------
# Finds the LMDB library
#
# This will define the following variables::
#
# LMDB_FOUND - system has LMDB
# LMDB_INCLUDE_DIRS - the LMDB include directory
# LMDB_LIBRARIES - the LMDB libraries
#
# and the following imported targets::
#
#   LMDB::LMDB   - The LMDB library

find_path(LMDB_INCLUDE_DIR NAMES lmdb.h)
find_library(LMDB_LIBRARY NAMES liblmdb.a liblmdb.lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB REQUIRED_VARS LMDB_LIBRARY LMDB_INCLUDE_DIR)

if(LMDB_FOUND)
  set(LMDB_INCLUDE_DIRS ${LMDB_INCLUDE_DIR})
  set(LMDB_LIBRARIES ${LMDB_LIBRARY})

  if(NOT TARGET LMDB::LMDB)
    add_library(LMDB::LMDB UNKNOWN IMPORTED)
    set_target_properties(LMDB::LMDB PROPERTIES
                                     IMPORTED_LOCATION "${LMDB_LIBRARY}")
  endif()
endif()

mark_as_advanced(LMDB_INCLUDE_DIR LMDB_LIBRARY)
