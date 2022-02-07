/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Identity.h"
#include "threads/CriticalSection.h"

namespace KODI
{
namespace CRYPTO
{

/*!
 * \ingroup crypto
 * \brief Helper class to have "C" struct created before other parts becomes his pointer.
 */
class CIdentityProviderStruct
{
public:
  CIdentityProviderStruct()
  {
    // Create "C" interface structures, used as own parts to prevent API problems on update
    KODI_ADDON_INSTANCE_INFO* info = new KODI_ADDON_INSTANCE_INFO();
    info->id = "";
    info->version = kodi::addon::GetTypeVersion(ADDON_INSTANCE_IDENTITY);
    info->type = ADDON_INSTANCE_IDENTITY;
    info->kodi = this;
    info->parent = nullptr;
    info->first_instance = true;
    info->functions = new KODI_ADDON_INSTANCE_FUNC_CB();
    m_ifc.info = info;
    m_ifc.functions = new KODI_ADDON_INSTANCE_FUNC();

    m_ifc.identity = new AddonInstance_Identity;
    m_ifc.identity->props = new AddonProps_Identity();
    m_ifc.identity->toKodi = new AddonToKodiFuncTable_Identity();
    m_ifc.identity->toAddon = new KodiToAddonFuncTable_Identity();
  }

  ~CIdentityProviderStruct()
  {
    delete m_ifc.functions;
    if (m_ifc.info)
      delete m_ifc.info->functions;
    delete m_ifc.info;
    if (m_ifc.identity)
    {
      delete m_ifc.identity->toAddon;
      delete m_ifc.identity->toKodi;
      delete m_ifc.identity->props;
      delete m_ifc.identity;
    }
  }

  KODI_ADDON_INSTANCE_STRUCT m_ifc;
};

/*!
 * \ingroup identity
 * \brief Interface between Kodi and Identity add-ons.
 */
class CIdentityAddonDLL : public ADDON::CAddonDll, private CIdentityProviderStruct
{
public:
  explicit CIdentityAddonDLL(const ADDON::AddonInfoPtr& addonInfo);

  ~CIdentityAddonDLL() override;

  // Implementation of IAddon via CAddonDll
  ADDON::AddonPtr GetRunningInstance() const override;

  // Start/stop identityplay
  bool Initialize(void);
  void Deinitialize();
  void Run();

  /*!
   * @brief To get the interface table used between addon and kodi
   * @todo This function becomes removed after old callback library system
   * is removed.
   */
  AddonInstance_Identity* GetInstanceInterface() { return m_ifc.identity; }

  // Helper functions
  void LogAddonProperties(void) const;

  /*!
   * @brief Callback functions from addon to kodi
   */
  //@{
  static void cb_close_identity(KODI_HANDLE kodiInstance);
  //@}

  CCriticalSection m_critSection;
};

} // namespace CRYPTO
} // namespace KODI
