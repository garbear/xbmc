set(SOURCES Multibase.cpp
            Multihash.cpp
            VarInt.cpp
)

set(HEADERS Multibase.h
            Multihash.h
            VarInt.h
)

core_add_library(crypto_multiformats)
