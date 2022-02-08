set(SOURCES TestIPFS.cpp
            TestIPFSBlockUtils.cpp
            TestIPFSDirectory.cpp
            TestIPFSFile.cpp
            TestIPFSUtils.cpp
            TestIPID.cpp
            TestIPLDJson.cpp
            TestUnixFSDirectory.cpp
            TestUnixFSJson.cpp)

core_add_test_library(filesystem_ipfs_test)
