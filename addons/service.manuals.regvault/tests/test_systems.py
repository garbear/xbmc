"""Which system a game's folder names, as collections spell their folders."""
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import systems  # noqa: E402


class FolderTest(unittest.TestCase):
    def folder(self, name):
        return systems.from_folder("/games/" + name + "/game.zip")

    def test_a_makers_name_in_front_does_not_hide_the_system(self):
        self.assertEqual(self.folder("Nintendo Game Boy"), "gb")
        self.assertEqual(self.folder("Nintendo Game Boy Color"), "gbc")
        self.assertEqual(self.folder("SNK - Neo Geo Pocket Color"), "ngpc")
        self.assertEqual(self.folder("Microsoft - MSX2"), "msx2")

    def test_the_folder_names_collections_use(self):
        self.assertEqual(self.folder("Atari 400 - 800"), "atari800")
        self.assertEqual(self.folder("Atari Jaguar CD"), "jaguarcd")
        self.assertEqual(self.folder("Magnavox - Odyssey2"), "odyssey2")
        self.assertEqual(self.folder("NEC - TurboGrafx-CD"), "pcenginecd")

    def test_a_folder_naming_no_system_names_none(self):
        self.assertIsNone(self.folder("Nintendo Switch"))
        self.assertIsNone(self.folder("Sonic The Hedgehog 2"))


if __name__ == "__main__":
    unittest.main()
