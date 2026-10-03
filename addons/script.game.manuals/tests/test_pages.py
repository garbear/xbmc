# -*- coding: utf-8 -*-
"""A manual's pages: where they are, their order, and a picture's size."""

import io
import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import pages  # noqa: E402


class PagesFolder(unittest.TestCase):
    def test_a_pdf_opens_through_vfs_pdf_as_kodi_names_it(self):
        self.assertEqual(pages.pages_folder("/games/manuals/Super Mario Bros. (USA, Europe).pdf"),
                         "pdf://%2fgames%2fmanuals%2fSuper%20Mario%20Bros.%20(USA%2c%20Europe).pdf/")

    def test_comic_archives_open_through_zip_and_rar(self):
        self.assertTrue(pages.pages_folder("/m/a.CBZ").startswith("zip://%2fm%2fa.CBZ"))
        self.assertTrue(pages.pages_folder("smb://box/m/a.cbr").startswith("rar://smb%3a%2f%2fbox"))

    def test_anything_else_has_no_pages_folder(self):
        self.assertEqual(pages.pages_folder("/m/a.txt"), "")


class Order(unittest.TestCase):
    def test_pictures_in_the_order_a_person_numbers_them(self):
        names = ["page10.jpg", "page2.jpg", "notes.txt", "page1.PNG", "Thumbs.db"]
        self.assertEqual(pages.ordered_pictures(names), ["page1.PNG", "page2.jpg", "page10.jpg"])


class PictureSize(unittest.TestCase):
    def test_png(self):
        header = b"\x89PNG\r\n\x1a\n" + struct.pack(">I", 13) + b"IHDR" + struct.pack(">II", 640, 480)
        self.assertEqual(pages.picture_size(header), (640, 480))

    def test_jpeg_past_its_metadata(self):
        app0 = b"\xff\xe0" + struct.pack(">H", 16) + b"JFIF\x00" + b"\x00" * 9
        sof2 = b"\xff\xc2" + struct.pack(">HBHH", 11, 8, 3200, 2560) + b"\x00" * 4
        self.assertEqual(pages.picture_size(b"\xff\xd8" + app0 + sof2), (2560, 3200))

    def test_a_real_jpeg(self):
        try:
            from PIL import Image
        except ImportError:
            self.skipTest("Pillow is not installed")
        out = io.BytesIO()
        Image.new("RGB", (123, 45)).save(out, "JPEG")
        self.assertEqual(pages.picture_size(out.getvalue()), (123, 45))

    def test_something_else(self):
        self.assertIsNone(pages.picture_size(b"not a picture"))


if __name__ == "__main__":
    unittest.main()
