set(SOURCES UnixFSJson.cpp
            UnixFSCodec.cpp
            UnixFSImporter.cpp
            UnixFSResolver.cpp
            UnixFSDirectory.cpp)

set(HEADERS UnixFSJson.h
            UnixFSJsonTypes.h
            UnixFSCodec.h
            UnixFSImporter.h
            UnixFSResolver.h
            UnixFSDirectory.h)

core_add_library(filesystem_ipfs_unixfs)

if(ENABLE_STATIC_LIBS)
  target_link_libraries(filesystem_ipfs_unixfs PUBLIC LIBRARY::ZSTD)
else()
  target_link_libraries(lib${APP_NAME_LC} PUBLIC LIBRARY::ZSTD)
endif()
