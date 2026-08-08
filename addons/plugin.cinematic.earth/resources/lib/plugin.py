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

import xbmcplugin


def main():
    """Finish the directory without adding any list items."""
    plugin_handle: int = int(sys.argv[1])
    xbmcplugin.endOfDirectory(plugin_handle)


if __name__ == "__main__":
    main()
