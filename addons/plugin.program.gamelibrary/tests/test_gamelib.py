#
#  Copyright (C) 2025 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Tests for the Game Library plugin."""

import os
import importlib.util
import tempfile
import unittest

PLUGIN_DIR = os.path.dirname(os.path.dirname(__file__))
spec = importlib.util.spec_from_file_location(
    "gamelib", os.path.join(PLUGIN_DIR, "gamelib.py")
)
gamelib = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gamelib)


class ScanGamesTest(unittest.TestCase):
    """Verify that scanning a directory returns all files."""

    def test_scan_games_lists_files(self):
        """scan_games should list all regular files in sorted order."""
        with tempfile.TemporaryDirectory() as tmpdir:
            paths = [os.path.join(tmpdir, name) for name in ["b.nes", "a.nes"]]
            for path in paths:
                with open(path, "w"):
                    pass
            result = gamelib.scan_games(tmpdir)
            self.assertEqual(sorted(paths), result)


if __name__ == "__main__":
    unittest.main()
