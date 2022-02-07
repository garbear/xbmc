set(SOURCES IdentityAddonDLL.cpp
            IdentityAddonPython.cpp
            IdentityProvider.cpp
)

set(HEADERS IdentityAddonDLL.h
            IdentityAddonPython.h
            IdentityProvider.h
)

core_add_library(crypto_identity)
