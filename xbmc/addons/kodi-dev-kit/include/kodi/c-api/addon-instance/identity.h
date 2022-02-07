/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_IDENTITY_H
#define C_API_ADDONINSTANCE_IDENTITY_H

#include "../addon_base.h"

#include <stddef.h> /* size_t */

//==============================================================================
/// @ingroup cpp_kodi_addon_identity_Defs
/// @brief **Port ID used when topology is unknown**
#define DEFAULT_PORT_ID "1"
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  /*!
   * @brief Identity properties
   *
   * Not to be used outside this header.
   */
  typedef struct AddonProps_Identity
  {
    /*!
     * The writable directory of the frontend
     */
    const char* profile_directory;
  } AddonProps_Identity;

  /*! Structure to transfer the methods from kodi_identity_dll.h to Kodi */

  struct AddonInstance_Identity;

  /*!
   * @brief Identity callbacks
   *
   * Not to be used outside this header.
   */
  typedef struct AddonToKodiFuncTable_Identity
  {
    KODI_HANDLE kodiInstance;

    void (*CloseIdentity)(KODI_HANDLE kodiInstance);
  } AddonToKodiFuncTable_Identity;

  /*!
   * @brief Identity function hooks
   *
   * Not to be used outside this header.
   */
  typedef struct KodiToAddonFuncTable_Identity
  {
    KODI_HANDLE addonInstance;

    bool(__cdecl* Initialize)(const struct AddonInstance_Identity*, const char*);
    void(__cdecl* Deinitialize)(const struct AddonInstance_Identity*);
    bool(__cdecl* Run)(const struct AddonInstance_Identity*);
  } KodiToAddonFuncTable_Identity;

  /*!
   * @brief Identity instance
   *
   * Not to be used outside this header.
   */
  typedef struct AddonInstance_Identity
  {
    struct AddonProps_Identity* props;
    struct AddonToKodiFuncTable_Identity* toKodi;
    struct KodiToAddonFuncTable_Identity* toAddon;
  } AddonInstance_Identity;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_IDENTITY_H */
