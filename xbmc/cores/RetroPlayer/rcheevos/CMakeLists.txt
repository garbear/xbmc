# rcheevos v12.3.0 — vendored as a static library
cmake_minimum_required(VERSION 3.5)

set(RCHEEVOS_SOURCES
  src/rc_client.c
  src/rc_compat.c
  src/rc_util.c
  src/rc_version.c
  src/rc_libretro.c
  src/rapi/rc_api_common.c
  src/rapi/rc_api_editor.c
  src/rapi/rc_api_info.c
  src/rapi/rc_api_runtime.c
  src/rapi/rc_api_user.c
  src/rcheevos/alloc.c
  src/rcheevos/condition.c
  src/rcheevos/condset.c
  src/rcheevos/consoleinfo.c
  src/rcheevos/format.c
  src/rcheevos/lboard.c
  src/rcheevos/memref.c
  src/rcheevos/operand.c
  src/rcheevos/richpresence.c
  src/rcheevos/runtime.c
  src/rcheevos/runtime_progress.c
  src/rcheevos/trigger.c
  src/rcheevos/value.c
  src/rhash/aes.c
  src/rhash/cdreader.c
  src/rhash/hash.c
  src/rhash/hash_disc.c
  src/rhash/hash_encrypted.c
  src/rhash/hash_rom.c
  src/rhash/hash_zip.c
  src/rhash/md5.c
)

add_library(rcheevos STATIC ${RCHEEVOS_SOURCES})

target_include_directories(rcheevos
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# RC_CLIENT_SUPPORTS_HASH enables rc_client_begin_identify_and_load_game()
# which hashes the ROM file automatically — essential for our integration
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH)

# Suppress warnings from third-party source
target_compile_options(rcheevos PRIVATE
  -w
)
