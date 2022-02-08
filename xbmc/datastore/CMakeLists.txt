set(SOURCES Block.cpp
            BlockStore.cpp
            CID.cpp
            DataStore.cpp
            DataStoreLMDB.cpp
)

set(HEADERS Block.h
            BlockStore.h
            CID.h
            DataStore.h
            DataStoreLMDB.h
            IDataStore.h
)

core_add_library(datastore)
