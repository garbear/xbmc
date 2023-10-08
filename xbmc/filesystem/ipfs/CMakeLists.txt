set(SOURCES IPFS.cpp
            IPFSData.cpp
            IPFSDirectory.cpp
            IPFSFile.cpp
            IPFSRepo.cpp
            UnixFSImporter.cpp
)

set(HEADERS IIPFS.h
            IPFS.h
            IPFSData.h
            IPFSDirectory.h
            IPFSFile.h
            IPFSRepo.h
            IUnixFS.h
            UnixFSImporter.h
)

core_add_library(filesystem_ipfs)
