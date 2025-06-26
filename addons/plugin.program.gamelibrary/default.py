#
#  Copyright (C) 2025 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Entry point for the Game Library plugin."""

import os
import sys
import urllib.parse
import xbmc
import xbmcgui
import xbmcplugin

from .gamelib import scan_games

ADDON_HANDLE = int(sys.argv[1])

GAMES_DIR = xbmc.translatePath("special://home/games")


def list_games():
    """Display game files as directory items."""
    for path in scan_games(GAMES_DIR):
        label = os.path.basename(path)
        list_item = xbmcgui.ListItem(label=label)
        url = f"{sys.argv[0]}?play={urllib.parse.quote(path)}"
        xbmcplugin.addDirectoryItem(handle=ADDON_HANDLE, url=url,
                                   listitem=list_item, isFolder=False)
    xbmcplugin.endOfDirectory(ADDON_HANDLE)


def play_game(path):
    """Launch the game file at *path* using Kodi."""
    xbmc.executebuiltin(f"PlayMedia({path})")


def router(paramstring):
    """Route incoming plugin requests."""
    params = dict(urllib.parse.parse_qsl(paramstring))
    if 'play' in params:
        play_game(params['play'])
    else:
        list_games()


if __name__ == '__main__':
    router(sys.argv[2][1:])
