set(SOURCES GameClientDiscModel.cpp
            GameClientDiscMergeUtils.cpp
            GameClientDiscs.cpp
            GameClientDiscTransport.cpp
            GameClientDiscXML.cpp
)

set(HEADERS GameClientDiscModel.h
            GameClientDiscMergeUtils.h
            GameClientDiscs.h
            GameClientDiscTransport.h
            GameClientDiscXML.h
)

core_add_library(gamediscs)
