/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/AddonCallbacks.h"
#include "addons/include/kodi_peripheral_types.h"

#include <stdio.h>

#ifdef _WIN32
  #include <windows.h>
  #define DLLEXPORT __declspec(dllexport)
#else
  #define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT CB_PeripheralLib* PERIPHERAL_register_me(AddonCB* frontend)
{
  CB_PeripheralLib* cb = NULL;
  if (!frontend)
    fprintf(stderr, "ERROR: PERIPHERAL_register_frontend is called with NULL handle!!!\n");
  else
  {
    cb = frontend->PeripheralLib_RegisterMe(frontend->addonData);
    if (!cb)
      fprintf(stderr, "ERROR: PERIPHERAL_register_frontend can't get callback table from frontend!!!\n");
  }
  return cb;
}

DLLEXPORT void PERIPHERAL_unregister_me(AddonCB* frontend, CB_PeripheralLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return frontend->PeripheralLib_UnRegisterMe(frontend->addonData, cb);
}

DLLEXPORT void PERIPHERAL_trigger_scan(AddonCB* frontend, CB_PeripheralLib* cb)
{
  if (frontend == NULL || cb == NULL)
    return;
  return cb->TriggerScan(frontend->addonData);
}

#ifdef __cplusplus
}
#endif
