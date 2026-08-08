################################################################################
#
#  Copyright (C) 2026 Rick Memsic
#  This file is part of cinematic.earth - https://cinematic.earth
#
#  SPDX-License-Identifier: AGPL-3.0-or-later
#  See the file LICENSE.txt for more information.
#
################################################################################

import sys

import xbmc
import xbmcaddon
import xbmcgui
import xbmcplugin


def main():
    """Populate the plugin directory from the add-on's service catalog."""
    plugin_handle: int = int(sys.argv[1])

    xbmcplugin.setContent(plugin_handle, "videos")

    try:
        catalog = xbmcaddon.Addon().getServiceCatalog()
    except RuntimeError as error:
        xbmc.log(
            f"plugin.cinematic.earth: Failed to load service catalog: {error}",
            xbmc.LOGERROR,
        )
        xbmcplugin.endOfDirectory(plugin_handle, succeeded=False)
        return

    items = catalog["items"] if catalog is not None else []
    for item in items:
        list_item = xbmcgui.ListItem(label=item["name"])
        list_item.setProperty("IsPlayable", "true")
        xbmcplugin.addDirectoryItem(
            plugin_handle,
            item["media"],
            list_item,
            isFolder=False,
        )

    xbmcplugin.endOfDirectory(plugin_handle)


if __name__ == "__main__":
    main()
