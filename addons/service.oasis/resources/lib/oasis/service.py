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
from oasis.windows.ocean_hud import OceanHUD
from oasis.windows.station_hud import StationHUD
from oasis.windows.swellpatrol_hud import SwellPatrolHUD
from oasis.windows.ventura_hud import VenturaHUD


# Window IDs
HOME_WINDOW_ID: int = 10000  # WINDOW_HOME

# Window properties
WINDOW_PROPERTY_HOSTNAME: str = "hostname"


class OasisService:
    @staticmethod
    def run(hostname: str) -> None:
        addon: xbmcaddon.Addon = xbmcaddon.Addon()
        addon_path: str = addon.getAddonInfo("path")

        xbmc.log(f"Running OASIS service on {hostname}", level=xbmc.LOGDEBUG)

        # Set the hostname property for the home window
        home_win = xbmcgui.Window(HOME_WINDOW_ID)
        home_win.setProperty(WINDOW_PROPERTY_HOSTNAME, hostname)

        window: xbmcgui.WindowXML

        # TODO: Hardware configuration
        HAS_CAMERA: bool = False
        if hostname == "bar":
            HAS_CAMERA = True

        if HAS_CAMERA:
            window = CameraView("CameraView1.xml", addon_path, "default", "1080i", False)
        else:
            window = SwellPatrolHUD("VideoHUD.xml", addon_path, "default", "1080i", False)

        window.doModal()
        xbmc.sleep(100)
        del window

        xbmc.log("Exiting OASIS service", level=xbmc.LOGDEBUG)
