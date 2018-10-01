set(SOURCES DataCacheCore.cpp
            FFmpeg.cpp
            PlaybackControl.cpp
            VideoSettings.cpp)

set(HEADERS DataCacheCore.h
            EdlEdit.h
            FFmpeg.h
            GameSettings.h
            IPlaybackControl.h
            IPlayer.h
            IPlayerCallback.h
            MenuType.h
            PlaybackControl.h
            VideoSettings.h)

core_add_library(cores)
