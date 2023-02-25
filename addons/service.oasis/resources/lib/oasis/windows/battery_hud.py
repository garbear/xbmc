################################################################################
#
#  Copyright (C) 2026 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import math
from threading import Event
from threading import Thread
from typing import Optional

import xbmc  # pylint: disable=import-error

from oasis.windows.swellpatrol_hud import SwellPatrolHUD


UPS_SYSTEMS = ("homeassistant", "door", "hallway", "oceanplatform")


def _reading(label: str, unit: str) -> Optional[float]:
    text = xbmc.getInfoLabel(label).strip().removesuffix(unit)
    try:
        value = float(text)
    except ValueError:
        return None
    return value if math.isfinite(value) and value >= 0 else None


class BatteryHUD(SwellPatrolHUD):
    def doModal(self) -> None:
        stopped = Event()
        worker = Thread(target=self._poll_ups, args=(stopped,))
        worker.start()
        try:
            super().doModal()
        finally:
            stopped.set()
            worker.join()

    def _poll_ups(self, stopped: Event) -> None:
        monitor = xbmc.Monitor()
        while not stopped.is_set() and not monitor.abortRequested():
            self._update_totals()
            stopped.wait(1.0)

    def _update_totals(self) -> None:
        charges = []
        loads = []
        for system in UPS_SYSTEMS:
            source = f"SmartHome.System({system})"
            if not xbmc.getCondVisibility(f"{source}.IsActive(2)"):
                continue

            charge = _reading(f"{source}.BatteryCharge", "%")
            if charge is not None and charge <= 100:
                charges.append(charge)
            load = _reading(f"{source}.BatteryLoad", "W")
            if load is not None:
                loads.append(load)

        if charges:
            mean = sum(charges) / len(charges)
            self.setProperty("TotalUPSCharge", f"{math.floor(mean + 0.5)} %")
        else:
            self.clearProperty("TotalUPSCharge")
        if loads:
            self.setProperty("TotalUPSLoad", f"{math.floor(sum(loads) + 0.5)} W")
        else:
            self.clearProperty("TotalUPSLoad")
