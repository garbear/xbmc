# -*- coding: utf-8 -*-
"""Finding a manual for a game that has none.

Each installed provider is asked in the background; picking a result fetches
it into the manuals folder beside the game, or into the profile when that
folder can't be written to, and the viewer then opens it.
"""

import threading

import xbmc
import xbmcgui
import xbmcvfs

from . import fetch, manuals, providers
from .main import jsonrpc, localize

RESULTS = 200
PROVIDER = 300
MORE = 310
CANCEL = 320
BAR_TRACK = 400
BAR_FILL = 410


def find_providers():
    """The installed add-ons that find manuals, as (id, name, icon)."""
    found = []
    answer = jsonrpc("Addons.GetAddons", {"type": "xbmc.python.pluginsource", "enabled": True,
                                          "properties": ["name", "path", "thumbnail"]})
    for addon in answer.get("addons") or []:
        manifest = manuals.join(addon.get("path") or "", "addon.xml")
        try:
            with xbmcvfs.File(manifest) as f:
                text = f.read()
        except Exception:
            continue
        if providers.provides_manuals(text):
            addon_id = addon["addonid"]
            found.append((addon_id, addon.get("name") or addon_id, addon.get("thumbnail") or ""))
    return found


class Finder(xbmcgui.WindowXMLDialog):
    def __init__(self, *args, **kwargs):
        super(Finder, self).__init__(*args)
        self.game = kwargs["game"]
        self.title = kwargs["title"]
        self.downloaded = ""
        self.providers = []
        self.provider = 0
        self.busy = False

    def onInit(self):
        self.setProperty("game", self.title)
        self.providers = find_providers()
        self.setProperty("providers", str(len(self.providers)))
        if not self.providers:
            self.set_state("noproviders", localize(32005), localize(32024))
            self.setFocusId(MORE)
            return
        self.search()

    def set_state(self, state, status="", hint=""):
        self.setProperty("state", state)
        self.setProperty("status", status)
        self.setProperty("hint", hint)

    def search(self):
        addon_id, name, icon = self.providers[self.provider]
        self.setProperty("provider", name)
        self.setProperty("provider.icon", icon)
        self.set_state("searching", localize(32023).format(name))
        self.getControl(RESULTS).reset()
        threading.Thread(target=self._search, args=(addon_id,)).start()

    def _search(self, addon_id):
        url = providers.search_url(addon_id, self.game, self.title)
        answer = jsonrpc("Files.GetDirectory", {"directory": url, "media": "files",
                                                "properties": ["title", "art", "customproperties"]})
        items = []
        for result in answer.get("files") or []:
            item = xbmcgui.ListItem(result.get("label") or result.get("title") or "")
            item.setArt(result.get("art") or {})
            item.setProperties(result.get("customproperties") or {})
            item.setProperty("url", result.get("file") or "")
            items.append(item)
        self.getControl(RESULTS).addItems(items)
        if items:
            self.set_state("results")
            self.setFocusId(RESULTS)
        else:
            self.set_state("empty", localize(32004), localize(32013))
            self.setFocusId(PROVIDER if len(self.providers) > 1 else MORE)

    def onClick(self, control_id):
        if self.busy:
            return
        if control_id == RESULTS:
            item = self.getControl(RESULTS).getSelectedItem()
            if item and item.getProperty("url"):
                threading.Thread(target=self.download, args=(item.getProperty("url"),)).start()
        elif control_id == PROVIDER and len(self.providers) > 1:
            self.provider = (self.provider + 1) % len(self.providers)
            self.search()
        elif control_id == MORE:
            self.close()
            xbmc.executebuiltin("ActivateWindow(addonbrowser,addons://search/manuals/,return)")
        elif control_id == CANCEL:
            self.close()

    def onAction(self, action):
        # Back is ignored mid-download: the file would be left half written
        if action.getId() in (10, 92) and not self.busy:
            self.close()

    def show_progress(self, done):
        self.setProperty("progress", "%d%%" % int(done * 100))
        try:
            track = self.getControl(BAR_TRACK).getWidth()
            self.getControl(BAR_FILL).setWidth(max(12, int(track * done)))
        except RuntimeError:
            # A skin's own window may show the progress as text alone
            pass

    def download(self, url):
        self.busy = True
        self.set_state("downloading")
        self.show_progress(0)
        target = fetch.download(url, self.game, self.show_progress)
        self.busy = False
        if not target:
            self.set_state("failed", localize(32008), localize(32025))
            self.setFocusId(CANCEL)
            return
        self.show_progress(1)
        self.downloaded = target
        self.close()
