# FindFmt
# -------
# Finds the libdatachannel library
#
# This will define the following variables::
#
# LIBDATACHANNEL_FOUND - system has libdatachannel
# LIBDATACHANNEL_INCLUDE_DIRS - the libdatachannel include directory
# LIBDATACHANNEL_LIBRARIES - the libdatachannel libraries
# LIBDATACHANNEL_DEFINITIONS - the libdatachannel compile definitions
#
# and the following imported targets::
#
#   LibDataChannel::LibDataChannel   - The libdatachannel library
#

include(FindPackageHandleStandardArgs)

find_package(Libjuice REQUIRED QUIET)
find_package(Libsrtp REQUIRED QUIET)
find_package(Plog REQUIRED QUIET)
find_package(Usrsctp REQUIRED QUIET)

if(NOT TARGET LibDataChannel::LibDataChannel)
  if(ENABLE_INTERNAL_LIBDATACHANNEL)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC libdatachannel)

    SETUP_BUILD_VARS()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0001-Look-for-usrsctp-in-subdirectory.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/0002-Rename-static-library-and-exclude-shared-library.patch")

    generate_patchcommand("${patches}")

    set(CMAKE_ARGS -DNO_EXAMPLES=ON
                   -DNO_TESTS=ON
                   -DPREFER_SYSTEM_LIB=ON
                   "${EXTRA_ARGS}")

    BUILD_DEP_TARGET()

    add_dependencies(${MODULE_LC} libjuice)
    add_dependencies(${MODULE_LC} libsrtp)
    add_dependencies(${MODULE_LC} plog)
    add_dependencies(${MODULE_LC} usrsctp)
  endif()
endif()

find_path(LIBDATACHANNEL_INCLUDE_DIR rtc/rtc.hpp)
find_library(LIBDATACHANNEL_LIBRARY NAMES datachannel)

find_package_handle_standard_args(LibDataChannel
  REQUIRED_VARS
    LIBDATACHANNEL_LIBRARY
    LIBDATACHANNEL_INCLUDE_DIR)

if(LIBDATACHANNEL_FOUND)
  set(LIBDATACHANNEL_INCLUDE_DIRS ${LIBDATACHANNEL_INCLUDE_DIR})
  set(LIBDATACHANNEL_LIBRARIES ${LIBDATACHANNEL_LIBRARY})
  set(LIBDATACHANNEL_DEFINITIONS -DRTC_STATIC)

  if(NOT TARGET LibDataChannel::LibDataChannel)
    add_library(LibDataChannel::LibDataChannel UNKNOWN IMPORTED)

    set_target_properties(LibDataChannel::LibDataChannel PROPERTIES
        IMPORTED_LOCATION "${LIBDATACHANNEL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBDATACHANNEL_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS "${LIBDATACHANNEL_DEFINITIONS}")

    add_dependencies(LibDataChannel::LibDataChannel Libjuice::Libjuice)
    add_dependencies(LibDataChannel::LibDataChannel libsrtp::libsrtp)
    add_dependencies(LibDataChannel::LibDataChannel plog::plog)
    add_dependencies(LibDataChannel::LibDataChannel Usrsctp::Usrsctp)

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibDataChannel::LibDataChannel)
  endif()
endif()

mark_as_advanced(LIBDATACHANNEL_INCLUDE_DIR LIBDATACHANNEL_LIBRARY)
