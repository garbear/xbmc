################################################################################
#
#  Copyright (C) 2022 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import xbmc  # pylint: disable=import-error
import xbmcaddon  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.windows.camera_view import CameraView
from oasis.windows.fireworks_hud import FireworksHUD
from oasis.windows.halloween_hud import HalloweenHUD
from oasis.windows.station_hud import StationHUD
from oasis.windows.ventura_hud import VenturaHUD


class OasisService:
    @staticmethod
    def run(hostname: str) -> None:
        addon: xbmcaddon.Addon = xbmcaddon.Addon()
        addon_path: str = addon.getAddonInfo("path")

        xbmc.log(f"Running OASIS service on {hostname}", level=xbmc.LOGDEBUG)

        window: xbmcgui.WindowXML

        # TODO: Hardware configuration
        if hostname == "cinder":
            window = HalloweenHUD(
                "HalloweenHUD.xml", addon_path, "default", "1080i", False
            )
        elif hostname == "lenovo":
            window = HalloweenHUD(
                "VerticalHUD.xml", addon_path, "default", "1080i", False
            )
        elif hostname == "substation":
            window = HalloweenHUD(
                "LabHUD.xml", addon_path, "default", "1080i", False
            )
        else:
            window = HalloweenHUD(
                "HalloweenHUD.xml", addon_path, "default", "1080i", False
            )

        window.doModal()
        xbmc.sleep(100)
        del window

        xbmc.log("Exiting OASIS service", level=xbmc.LOGDEBUG)
