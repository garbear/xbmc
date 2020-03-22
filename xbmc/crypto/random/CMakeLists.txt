set(SOURCES BoostRandomGenerator.cpp
)

set(HEADERS BoostRandomGenerator.h
            IRandomGenerator.h
)

core_add_library(crypto_random)

target_link_libraries(crypto_random
  Boost::random
)
