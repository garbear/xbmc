################################################################################
#
#  Copyright (C) 2023 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import os
from typing import Dict
from typing import List
from typing import Union

import xbmc  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.utils.playlist_player import PlaylistPlayer


# Kodi constants
MEDIA_TYPE_MOVIE: str = "movie"

# Get the current user's home directory and set the asset directory
HOME_DIR: str = os.path.expanduser("~")
ASSET_DIR: str = os.path.join(HOME_DIR, "Videos", "Halloween")

# List of assets and metadata
ASSETS: List[Dict[str, Union[str, int]]] = [
    {
        "title": "Scream",
        "year": 1996,
        "foldername": "Scream (1996) [Remastered]",
        "filename": "Scream 1996 REMASTERED BluRay 1080p DTS AC3 x264-MgB.mkv",
        "poster": "Scream 1996 REMASTERED BluRay 1080p DTS AC3 x264-MgB-poster.jpg",
    },
    # {
    #    "title": "Jaws",
    #    "year": "1975",
    #    "foldername": "Jaws (1975) [Remastered]",
    #    "filename": "Jaws 1975 Remastered -  Eng Subs 1080p [H264-mp4].mp4",
    #    "poster": "Jaws 1975 Remastered -  Eng Subs 1080p [H264-mp4]-poster.jpb",
    # },
]


class HalloweenHUD(xbmcgui.WindowXML):
    def onInit(self) -> None:
        """Service entry-point."""
        self._play()

    @staticmethod
    def _play() -> None:
        # Check if the asset directory exists
        if not os.path.exists(ASSET_DIR):
            xbmc.log(
                f"Asset directory does not exist: {ASSET_DIR}", level=xbmc.LOGERROR
            )
            return

        #
        playlist: xbmc.PlayList = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)

        # Add assets to playlist
        for asset in ASSETS:
            asset_folder: str = os.path.join(ASSET_DIR, str(asset["foldername"]))
            asset_path: str = os.path.join(asset_folder, str(asset["filename"]))

            list_item: xbmcgui.ListItem = xbmcgui.ListItem(str(asset["title"]))
            list_item.setArt(
                {"poster": os.path.join(asset_folder, str(asset["poster"]))}
            )

            video_info_tag = list_item.getVideoInfoTag()
            video_info_tag.setMediaType(MEDIA_TYPE_MOVIE)
            video_info_tag.setTitle(str(asset["title"]))
            video_info_tag.setYear(int(asset["year"]))

            playlist.add(url=asset_path, listitem=list_item)

        playlist.shuffle()

        PlaylistPlayer.play_playlist(playlist)
