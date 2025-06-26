#
#  Copyright (C) 2025 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Utility functions for the Game Library plugin."""

import os


def scan_games(directory):
    """Return a sorted list of game file paths in *directory*."""
    if not os.path.isdir(directory):
        return []
    games = [os.path.join(directory, f) for f in os.listdir(directory)
             if os.path.isfile(os.path.join(directory, f))]
    return sorted(games)
