set(SOURCES DIDConstants.cpp
            DIDDocument.cpp
            DIDKeyDriver.cpp
            DIDUtils.cpp
            IPID.cpp
)

set(HEADERS DIDConstants.h
            DIDDocument.h
            DIDKeyDriver.h
            DIDUtils.h
            IPID.h
)

core_add_library(crypto_did)
