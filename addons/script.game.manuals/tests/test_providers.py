# -*- coding: utf-8 -*-
"""Recognising a manual provider, and asking one."""

import os
import sys
import unittest
from urllib.parse import parse_qs, urlsplit

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources"))

from lib import providers  # noqa: E402

REGVAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                        "service.manuals.regvault", "addon.xml")


class ProvidesManuals(unittest.TestCase):
    def test_either_way_of_saying_it(self):
        attribute = ('<addon id="a"><extension point="xbmc.python.pluginsource" library="a.py" '
                     'provides="game manuals"/></addon>')
        element = ('<addon id="a"><extension point="xbmc.python.pluginsource" library="a.py">'
                   '<provides>manuals</provides></extension></addon>')
        self.assertTrue(providers.provides_manuals(attribute))
        self.assertTrue(providers.provides_manuals(element))

    def test_other_plugins_are_not_providers(self):
        video = ('<addon id="a"><extension point="xbmc.python.pluginsource" library="a.py">'
                 '<provides>video</provides></extension></addon>')
        self.assertFalse(providers.provides_manuals(video))
        self.assertFalse(providers.provides_manuals("not xml"))

    def test_regvault_is_one(self):
        if not os.path.exists(REGVAULT):
            self.skipTest("service.manuals.regvault is not checked out beside this")
        with open(REGVAULT, encoding="utf-8") as f:
            self.assertTrue(providers.provides_manuals(f.read()))


class SearchUrl(unittest.TestCase):
    def test_the_path_and_title_are_handed_over(self):
        url = providers.search_url("service.manuals.regvault", "/games/Super Mario Bros. (USA, Europe).nes")
        parts = urlsplit(url)
        self.assertEqual((parts.scheme, parts.netloc), ("plugin", "service.manuals.regvault"))
        self.assertEqual(parse_qs(parts.query), {"action": ["search"],
                                                 "path": ["/games/Super Mario Bros. (USA, Europe).nes"],
                                                 "title": ["Super Mario Bros."]})

    def test_a_title_given_is_the_hint(self):
        url = providers.search_url("p", "/g/smb.nes", "Super Mario Bros.")
        self.assertEqual(parse_qs(urlsplit(url).query)["title"], ["Super Mario Bros."])


if __name__ == "__main__":
    unittest.main()
