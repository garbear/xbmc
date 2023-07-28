# FindFmt
# -------
# Finds the usrsctp library
#
# This will define the following variables::
#
# USRSCTP_FOUND - system has usrsctp
# USRSCTP_INCLUDE_DIRS - the usrsctp include directory
# USRSCTP_LIBRARIES - the usrsctp libraries
#
# and the following imported targets::
#
#   Usrsctp::Usrsctp   - The usrsctp library
#

include(FindPackageHandleStandardArgs)

if(NOT TARGET Usrsctp::Usrsctp)
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC usrsctp)

    SETUP_BUILD_VARS()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Add-CMake-export-targets.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Also-install-library-to-subdirectory.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0003-fix-build-error-on-windows.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -Dsctp_build_programs=OFF
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()
  endif()
endif()

find_path(USRSCTP_INCLUDE_DIR usrsctp.h)
find_library(USRSCTP_LIBRARY NAMES usrsctp)

find_package_handle_standard_args(Usrsctp
  REQUIRED_VARS
    USRSCTP_LIBRARY
    USRSCTP_INCLUDE_DIR)

if(USRSCTP_FOUND)
  set(USRSCTP_INCLUDE_DIRS ${USRSCTP_INCLUDE_DIR})
  set(USRSCTP_LIBRARIES ${USRSCTP_LIBRARY})

  if(NOT TARGET Usrsctp::Usrsctp)
    add_library(Usrsctp::Usrsctp UNKNOWN IMPORTED)

    set_target_properties(Usrsctp::Usrsctp PROPERTIES
        IMPORTED_LOCATION "${USRSCTP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${USRSCTP_INCLUDE_DIR}")

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Usrsctp::Usrsctp)
  endif()
endif()

mark_as_advanced(USRSCTP_INCLUDE_DIR USRSCTP_LIBRARY)
