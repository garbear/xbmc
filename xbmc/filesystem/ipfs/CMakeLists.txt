set(SOURCES IPFS.cpp
            IPFSFile.cpp
            IPFSDirectory.cpp
            IPFSService.cpp
            IPFSUtils.cpp
            UnixFS.cpp
)

set(HEADERS IIPFS.h
            IPFS.h
            IPFSFile.h
            IPFSDirectory.h
            IPFSService.h
            IPFSUtils.h
            UnixFS.h
            UnixFSTypes.h
)

core_add_library(filesystem_ipfs)
