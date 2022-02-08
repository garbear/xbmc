set(SOURCES Multibase.cpp
            Multicodec.cpp
            Multihash.cpp
            VarInt.cpp
)

set(HEADERS Multibase.h
            Multicodec.h
            Multihash.h
            VarInt.h
)

core_add_library(crypto_multiformats)
