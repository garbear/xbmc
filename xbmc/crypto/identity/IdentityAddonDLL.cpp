/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IdentityAddonDLL.h"

#include "ServiceBroker.h"
#include "addons/AddonVersion.h"
#include "addons/BinaryAddonCache.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;
using namespace CRYPTO;

CIdentityAddonDLL::CIdentityAddonDLL(const ADDON::AddonInfoPtr& addonInfo)
  : CAddonDll(addonInfo, ADDON::AddonType::IDENTITYDLL)
{
}

CIdentityAddonDLL::~CIdentityAddonDLL(void)
{
  Deinitialize();
}

ADDON::AddonPtr CIdentityAddonDLL::GetRunningInstance() const
{
  using namespace ADDON;

  CBinaryAddonCache& addonCache = CServiceBroker::GetBinaryAddonCache();
  return addonCache.GetAddonInstance(ID(), Type());
}

bool CIdentityAddonDLL::Initialize(void)
{
  using namespace XFILE;

  // Ensure user profile directory exists for add-on
  if (!CDirectory::Exists(Profile()))
    CDirectory::Create(Profile());

  //if (!AddonProperties().InitializeProperties())
  //  return false;

  m_ifc.identity->toKodi->kodiInstance = this;
  m_ifc.identity->toKodi->CloseIdentity = cb_close_identity;

  memset(m_ifc.identity->toAddon, 0, sizeof(KodiToAddonFuncTable_Identity));

  if (CreateInstance(&m_ifc) == ADDON_STATUS_OK)
  {
    LogAddonProperties();
    return true;
  }

  return false;
}

void CIdentityAddonDLL::Deinitialize()
{
}

void CIdentityAddonDLL::Run()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_ifc.identity->toAddon->Run(m_ifc.identity);
}

void CIdentityAddonDLL::LogAddonProperties(void) const
{
  CLog::Log(LOGINFO, "IDENTITY: ------------------------------------");
  CLog::Log(LOGINFO, "IDENTITY: Loaded DLL for {}", ID());
  CLog::Log(LOGINFO, "IDENTITY: Client:  {}", Name());
  CLog::Log(LOGINFO, "IDENTITY: Version: {}", Version().asString());
  CLog::Log(LOGINFO, "IDENTITY: ------------------------------------");
}

void CIdentityAddonDLL::cb_close_identity(KODI_HANDLE kodiInstance)
{
  //! @todo
}
