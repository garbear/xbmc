set(SOURCES FileStore.cpp
            MediaCache.cpp
            MediaContainer.cpp
            MediaStore.cpp
            MediaStoreDatabase.cpp
            MediaStream.cpp
            MediaType.cpp
            MuxerFFmpeg.cpp)

set(HEADERS FileStore.h
            IMuxer.h
            MediaCache.h
            MediaContainer.h
            MediaStore.h
            MediaStoreDatabase.h
            MediaStream.h
            MediaType.h
            MuxerFFmpeg.h)

core_add_library(media)
