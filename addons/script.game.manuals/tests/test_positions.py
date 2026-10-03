# -*- coding: utf-8 -*-
"""Reopening a manual where it was left."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import positions  # noqa: E402


class Positions(unittest.TestCase):
    def setUp(self):
        self.files = {}

    def store(self):
        return positions.Positions("p.json", self.files.get, self.files.__setitem__)

    def test_a_manual_reopens_at_its_page(self):
        self.store().remember("/m/a.pdf", 7)
        self.assertEqual(self.store().page("/m/a.pdf"), 7)

    def test_an_unread_manual_opens_at_the_cover(self):
        self.assertEqual(self.store().page("/m/new.pdf"), 0)

    def test_closing_on_the_cover_forgets_it(self):
        s = self.store()
        s.remember("/m/a.pdf", 3)
        s.remember("/m/a.pdf", 0)
        self.assertNotIn("/m/a.pdf", self.files["p.json"])

    def test_a_damaged_file_is_a_fresh_start(self):
        self.files["p.json"] = "{not json"
        self.assertEqual(self.store().page("/m/a.pdf"), 0)

    def test_the_oldest_are_dropped(self):
        s = self.store()
        for i in range(positions.KEPT + 5):
            s.remember("/m/%d.pdf" % i, 1)
        self.assertEqual(len(s.pages), positions.KEPT)
        self.assertEqual(s.page("/m/0.pdf"), 0)
        self.assertEqual(s.page("/m/%d.pdf" % (positions.KEPT + 4)), 1)


if __name__ == "__main__":
    unittest.main()
