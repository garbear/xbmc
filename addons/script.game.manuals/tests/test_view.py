# -*- coding: utf-8 -*-
"""Where a page sits on screen as it is zoomed and panned."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import view  # noqa: E402


class WholePage(unittest.TestCase):
    def test_a_portrait_page_fits_the_height_and_is_centred(self):
        v = view.View(1920, 1080)
        v.show(2000, 3000)
        self.assertEqual(v.rect(), (600, 0, 720, 1080))

    def test_a_spread_fits_the_width(self):
        v = view.View(1920, 1080)
        v.show(4000, 1000)
        self.assertEqual(v.rect(), (0, 300, 1920, 480))

    def test_panning_a_whole_page_moves_nothing(self):
        v = view.View(1920, 1080)
        v.show(2000, 3000)
        self.assertFalse(v.pan(1, 0))
        self.assertFalse(v.pan(0, 1))


class Zoom(unittest.TestCase):
    def setUp(self):
        self.v = view.View(1920, 1080)
        self.v.show(2000, 3000)

    def test_zooming_keeps_the_middle_of_the_page_in_the_middle(self):
        self.assertTrue(self.v.step(+2))
        x, y, w, h = self.v.rect()
        self.assertEqual((w, h), (1440, 2160))
        self.assertEqual((x + w / 2, y + h / 2), (960, 540))

    def test_zoom_stops_at_either_end(self):
        self.assertFalse(self.v.step(-1))
        self.assertTrue(self.v.step(+10))
        self.assertFalse(self.v.step(+1))

    def test_panning_stops_at_the_edge_of_the_page(self):
        self.v.step(+2)
        while self.v.pan(0, -1):
            pass
        self.assertEqual(self.v.rect()[1], 0, "the top of the page is at the top of the screen")
        while self.v.pan(0, 1):
            pass
        x, y, w, h = self.v.rect()
        self.assertEqual(y + h, 1080, "and the bottom at the bottom")

    def test_a_page_narrower_than_the_screen_stays_centred_across(self):
        self.v.step(+1)
        self.assertFalse(self.v.pan(1, 0))
        x, y, w, h = self.v.rect()
        self.assertEqual(x + w / 2, 960)

    def test_whole_puts_it_back(self):
        self.v.step(+3)
        self.v.pan(0, 2)
        self.assertTrue(self.v.whole())
        self.assertEqual(self.v.rect(), (600, 0, 720, 1080))
        self.assertTrue(self.v.is_whole)


if __name__ == "__main__":
    unittest.main()
