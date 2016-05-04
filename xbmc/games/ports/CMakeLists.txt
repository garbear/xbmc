set(SOURCES PortManager.cpp
            PortMapper.cpp)

set(HEADERS PortManager.h
            PortMapper.h)

core_add_library(gameports)
add_dependencies(gameports libcpluff)
