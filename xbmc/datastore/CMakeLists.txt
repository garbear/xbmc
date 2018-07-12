set(SOURCES DataStore.cpp
            DataStoreLMDB.cpp
)

set(HEADERS DataStore.h
            DataStoreLMDB.h
            IDataStore.h
)

core_add_library(datastore)
