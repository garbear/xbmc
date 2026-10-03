# -*- coding: utf-8 -*-
"""Downloaded manuals kept within the space the player chose."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import budget  # noqa: E402

MB = budget.MEGABYTE


class Budget(unittest.TestCase):
    def setUp(self):
        self.files = {}
        self.disk = set()

    def store(self):
        return budget.Budget("b.json", self.files.get, self.files.__setitem__)

    def fetched(self, store, manual, megabytes, when):
        self.disk.add(manual)
        store.add(manual, megabytes * MB, when)

    def delete(self, manual):
        self.disk.discard(manual)
        return True

    def test_the_least_recently_opened_go_first(self):
        s = self.store()
        self.fetched(s, "/m/a.pdf", 100, 1)
        self.fetched(s, "/m/b.pdf", 100, 2)
        self.fetched(s, "/m/c.pdf", 100, 3)
        s.opened("/m/a.pdf", 4)
        self.assertEqual(s.enforce(200 * MB, "", self.disk.__contains__, self.delete), ["/m/b.pdf"])
        self.assertEqual(self.disk, {"/m/a.pdf", "/m/c.pdf"})

    def test_the_manual_about_to_be_read_is_spared(self):
        s = self.store()
        self.fetched(s, "/m/old.pdf", 100, 1)
        self.fetched(s, "/m/new.pdf", 300, 2)
        deleted = s.enforce(250 * MB, "/m/new.pdf", self.disk.__contains__, self.delete)
        self.assertEqual(deleted, ["/m/old.pdf"])
        self.assertIn("/m/new.pdf", self.disk)

    def test_a_players_own_manual_is_never_touched(self):
        s = self.store()
        self.disk.add("/m/mine.pdf")
        s.opened("/m/mine.pdf", 5)
        self.fetched(s, "/m/fetched.pdf", 300, 1)
        s.enforce(0, "", self.disk.__contains__, self.delete)
        self.assertIn("/m/mine.pdf", self.disk)
        self.assertNotIn("/m/mine.pdf", s.manuals)

    def test_a_manual_deleted_by_hand_is_forgotten(self):
        s = self.store()
        self.fetched(s, "/m/a.pdf", 100, 1)
        self.disk.discard("/m/a.pdf")
        self.assertEqual(s.enforce(0, "", self.disk.__contains__, self.delete), [])
        self.assertEqual(s.total(), 0)

    def test_one_that_cannot_be_deleted_stays_counted(self):
        s = self.store()
        self.fetched(s, "/m/a.pdf", 100, 1)
        self.assertEqual(s.enforce(0, "", self.disk.__contains__, lambda m: False), [])
        self.assertEqual(s.total(), 100 * MB)

    def test_it_survives_a_restart_and_a_damaged_file(self):
        s = self.store()
        self.fetched(s, "/m/a.pdf", 10, 1)
        self.assertEqual(self.store().total(), 10 * MB)
        self.files["b.json"] = "{broken"
        self.assertEqual(self.store().total(), 0)


if __name__ == "__main__":
    unittest.main()
