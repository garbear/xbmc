set(SOURCES OpenSSLRandomGenerator.cpp
)

set(HEADERS IRandomGenerator.h
            OpenSSLRandomGenerator.h
)

core_add_library(crypto_random)
