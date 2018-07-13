set(SOURCES Block.cpp
            BlockStore.cpp
            CID.cpp
            DataStore.cpp
            DataStoreLMDB.cpp
            Multihash.cpp
)

set(HEADERS Block.h
            BlockStore.h
            CID.h
            DataStore.h
            DataStoreLMDB.h
            IDataStore.h
            Multihash.h
)

core_add_library(datastore)
