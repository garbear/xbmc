set(SOURCES Ed25519VerificationKey2020.cpp
            OpenSSLEd25519Provider.cpp
)

set(HEADERS Ed25519VerificationKey2020.h
            IEd25519Provider.h
            OpenSSLEd25519Provider.h
)

core_add_library(crypto_ed25519)
