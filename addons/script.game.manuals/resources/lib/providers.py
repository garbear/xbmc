# -*- coding: utf-8 -*-
"""Add-ons that find manuals, and how they are asked.

A provider is an ordinary plugin, called the way Kodi calls a subtitle service,
with a directory listing for an answer:

  plugin://<provider>/?action=search&path=<game>&title=<name>

Each result's path is the manual itself, to be copied wherever it is wanted.
A plugin says it is one by listing "manuals" among what it provides.
"""

import xml.etree.ElementTree as ET
from urllib.parse import urlencode

from . import manuals

PROVIDES = "manuals"


def provides_manuals(addon_xml):
    """Whether an add-on's addon.xml marks it as a manual provider."""
    try:
        root = ET.fromstring(addon_xml)
    except ET.ParseError:
        return False
    for extension in root.iter("extension"):
        if extension.get("point") != "xbmc.python.pluginsource":
            continue
        tokens = (extension.get("provides") or "").split()
        for provides in extension.iter("provides"):
            tokens += (provides.text or "").split()
        if PROVIDES in tokens:
            return True
    return False


def search_url(addon_id, game_path, title=""):
    """What to list to ask a provider for this game's manual."""
    # The path is what settles it: how a provider decides what matches is its
    # own business, and the title is only a hint
    query = urlencode({"action": "search", "path": game_path,
                       "title": title or manuals.display_title(game_path)})
    return "plugin://%s/?%s" % (addon_id, query)
