################################################################################
#
#  Copyright (C) 2026 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

from threading import Event
from threading import Thread

import xbmc  # pylint: disable=import-error

from oasis.windows.swellpatrol_hud import SwellPatrolHUD


class TrainHUD(SwellPatrolHUD):
    def doModal(self) -> None:
        stopped = Event()
        worker = Thread(target=self._poll_controller, args=(stopped,))
        worker.start()
        try:
            super().doModal()
        finally:
            stopped.set()
            worker.join()

    def _poll_controller(self, stopped: Event) -> None:
        monitor = xbmc.Monitor()
        while not stopped.is_set() and not monitor.abortRequested():
            battery = xbmc.getInfoLabel("SmartHome.System(megapegasus).ControllerBattery")
            self.setProperty("ControllerBattery", battery)
            stopped.wait(1.0)
