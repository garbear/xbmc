set(SOURCES DIDConstants.cpp
            DIDDocument.cpp
            DIDKeyDriver.cpp
            DIDUtils.cpp
)

set(HEADERS DIDConstants.h
            DIDDocument.h
            DIDKeyDriver.h
            DIDUtils.h
)

core_add_library(crypto_did)
