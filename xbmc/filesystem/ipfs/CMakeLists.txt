set(SOURCES IPFS.cpp
            IPFSFile.cpp
            IPFSDirectory.cpp
            IPFSService.cpp
            IPFSUtils.cpp
            ZstdCompression.cpp
)

set(HEADERS IIPFS.h
            IPFS.h
            IPFSFile.h
            IPFSDirectory.h
            IPFSService.h
            IPFSUtils.h
            ZstdCompression.h
)

core_add_library(filesystem_ipfs)

if(ENABLE_STATIC_LIBS)
  target_link_libraries(filesystem_ipfs PUBLIC LIBRARY::ZSTD)
else()
  target_link_libraries(lib${APP_NAME_LC} PUBLIC LIBRARY::ZSTD)
endif()
