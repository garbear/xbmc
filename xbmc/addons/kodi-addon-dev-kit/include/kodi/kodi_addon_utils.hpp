/*
 *      Copyright (C) 2013-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 #pragma once

/*!
 * @file kodi_addon_utils.hpp  C++ wrappers for add-on callbacks
 */

#include "libXBMC_addon.h"

#include <cstring>
#include <string>

class AddonUtils
{
public:
  /*!
   * @brief Convert a string of unknown encoding to UTF8
   * @param frontend   The add-on callback helper
   * @param str        The string
   * @return The UTF8 string, or empty if the add-on helper fails to return a string
   */
  static std::string UnknownToUTF8(ADDON::CHelper_libXBMC_addon* frontend, const std::string& str)
  {
    std::string strReturn;
    if (frontend)
    {
      char* strMsg = frontend->UnknownToUTF8(str.c_str());
      if (strMsg)
      {
        if (std::strlen(strMsg))
          strReturn = strMsg;
        frontend->FreeString(strMsg);
      }
    }
    return strReturn;
  }

  /*!
   * @brief Get a localized message
   * @param frontend   The add-on callback helper
   * @param dwCode     The code of the message to get
   * @param strDefault The default message, also helps to identify the code that is used
   * @return The localized message, or strDefault if the add-on helper fails to return a message
   */
  static std::string GetLocalizedString(ADDON::CHelper_libXBMC_addon* frontend, int dwCode, const std::string& strDefault = "")
  {
    std::string strReturn(strDefault);
    if (frontend)
    {
      char* strMsg = frontend->GetLocalizedString(dwCode);
      // frontend->GetLocalizedString() will return an empty string on invalid ID (such as 31000)
      // If the default is non-empty, we were probably expecting a non-empty message, so
      // return the default in this case
      if (strMsg)
      {
        if (std::strlen(strMsg))
          strReturn = strMsg;
        frontend->FreeString(strMsg);
      }
    }
    return strReturn;
  }

  /*!
   * @brief Get the DVD menu language
   * @param frontend   The add-on callback helper
   * @return The DVD menu langauge, or empty if unknown
   */
  static std::string GetDVDMenuLanguage(ADDON::CHelper_libXBMC_addon* frontend)
  {
    std::string strReturn;
    if (frontend)
    {
      char* strMsg = frontend->GetDVDMenuLanguage();
      if (strMsg)
      {
        if (std::strlen(strMsg))
          strReturn = strMsg;
        frontend->FreeString(strMsg);
      }
    }
    return strReturn;
  }

  /*!
   * @brief Translate an add-on status return code into a human-readable string
   * @param status The return code
   * @return A human-readable string suitable for logging
   */
  static const char* TranslateAddonStatus(ADDON_STATUS status)
  {
    switch (status)
    {
      case ADDON_STATUS_OK:                 return "OK";
      case ADDON_STATUS_LOST_CONNECTION:    return "Lost connection";
      case ADDON_STATUS_NEED_RESTART:       return "Needs restart";
      case ADDON_STATUS_NEED_SETTINGS:      return "Needs settngs";
      case ADDON_STATUS_UNKNOWN:            return "Unknown";
      case ADDON_STATUS_NEED_SAVEDSETTINGS: return "Needs saved settings";
      case ADDON_STATUS_PERMANENT_FAILURE:  return "Permanent failure";
      default:
        break;
    }
    return "";
  }
};
