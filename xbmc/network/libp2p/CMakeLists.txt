set(SOURCES ILibp2p.cpp
)

set(HEADERS ILibp2p.h
)

if (ENABLE_LIBP2P)
  list(APPEND SOURCES
    Libp2p.cpp
  )
  list(APPEND HEADERS
    Libp2p.h
  )
endif()

core_add_library(network_libp2p)
