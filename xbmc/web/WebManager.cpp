/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/WindowIDs.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"

#include "WebAddon.h"
#include "WebDatabase.h"
#include "WebManager.h"
#include "windows/GUIWindowWebBrowser.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWebAddonControl.h"

using namespace WEB;
using namespace ADDON;
using namespace ANNOUNCEMENT;
using namespace KODI::MESSAGING;

class CWebStartupJob : public CJob
{
public:
  CWebStartupJob(void) = default;
  ~CWebStartupJob() override = default;
  const char *GetType() const override { return "web-startup"; }

  bool DoWork() override;
};

bool CWebStartupJob::DoWork(void)
{
  CServiceBroker::GetWEBManager().Init(true);
  return true;
}

CWebManager::CWebManager(void)
  : m_addonControl(this)
{
}

CWebManager::~CWebManager(void)
{
  Deinit();
}

void CWebManager::Init(bool thisThread/* = false*/)
{
  if (!thisThread)
  {
    CJobManager::GetInstance().AddJob(new CWebStartupJob(), nullptr);
    return;
  }

  Deinit();
  UpdateAddons();
  m_started = true;
}

void CWebManager::Stop()
{
  if (!m_started)
    return;

  // Before system becomes deinitialized and GUI, audio manager ... destroyed
  // make sure there comes no stream from addon
  SetMute(true);
}

void CWebManager::Deinit()
{
  if (!m_started)
    return;

  MainShutdown();
  Clear();
  m_started = false;
}

bool CWebManager::MainInitialize()
{
  CSingleLock lock(m_critSection);

  if (!m_activeWebbrowser)
    return false;
  return m_activeWebbrowser->MainInitialize();
}

void CWebManager::MainLoop()
{
  CSingleLock lock(m_critSection);

  if (m_activeWebbrowser)
    m_activeWebbrowser->MainLoop();
}

void CWebManager::MainShutdown()
{
  CSingleLock lock(m_critSection);

  CGUIWindowWebBrowser *runningWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowWebBrowser>(WINDOW_WEB_BROWSER);
  if (runningWindow)
  {
    runningWindow->ControlDestroyAll();
  }

  m_addonControl.Clear();

 if (m_activeWebbrowser)
   m_activeWebbrowser->MainShutdown();
}

void CWebManager::SetMute(bool muted)
{
  CSingleLock lock(m_critSection);

  if (m_activeWebbrowser)
    m_activeWebbrowser->SetMute(muted);
}

bool CWebManager::IsInUse(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);

  for (const auto& addon : m_addonMap)
    if (!CServiceBroker::GetAddonMgr().IsAddonDisabled(addon.second->ID()) && addon.second->ID() == addonId)
      return true;
  return false;
}

bool CWebManager::IsCreatedAddon(const BinaryAddonBasePtr& addon)
{
  CSingleLock lock(m_critSection);

  for (const auto &entry : m_addonMap)
    if (entry.second->ID() == addon->ID())
      return entry.second->ReadyToUse();
  return false;
}

void CWebManager::Clear(void)
{
  CSingleLock lock(m_critSection);

  for (const auto& control : m_runningGUIControls)
  {
    control->SetWebAddonInvalid();
  }

  m_activeWebbrowser.reset();
  m_addonNameIds.clear();
  m_addonMap.clear();
}

bool CWebManager::IsWebWindow(int windowId)
{
  return (windowId == WINDOW_WEB_BROWSER);
}

bool CWebManager::IsWebWindowActive(void) const
{
  return IsWebWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
}

void CWebManager::LocalizationChanged(std::string strLanguage)
{
  CSingleLock lock(m_critSection);

  for (WEB_ADDON_MAP_CITR it = m_addonMap.begin(); it != m_addonMap.end(); ++it)
    it->second->SetLanguage(strLanguage);
}











void CWebManager::EnableEvent(ADDON::BinaryAddonBasePtr addon)
{
  UpdateAddons();
}

void CWebManager::DisableEvent(ADDON::BinaryAddonBasePtr addon)
{
  UpdateAddons();
//   UnRegisterAddon(addon->ID());
}

void CWebManager::UpdateEvent(ADDON::BinaryAddonBasePtr addon)
{
  UpdateAddons();
//   UnRegisterAddon(addon->ID());
}

void CWebManager::InstallEvent(ADDON::BinaryAddonBasePtr addon, bool update, bool modal)
{
  UpdateAddons();
}

void CWebManager::PreUnInstallEvent(ADDON::BinaryAddonBasePtr addon)
{
  UpdateAddons();
//   UnRegisterAddon(addon->ID());
}






















// void CWebManager::Stop(void)
// {
// //   if (IsStopped())
// //     return;
// //
// //   SetState(ManagerStateStopping);
// //
// //   CLog::Log(LOGNOTICE, "CWebManager::%s: stopping", __FUNCTION__);
// //
// //   /* close database */
// //   const CWebDatabasePtr database(GetDatabase());
// //   if (database && database->IsOpen())
// //     database->Close();
// //
// //   SetState(ManagerStateStopped);
// }






























CWebManager::ManagerState CWebManager::GetState(void) const
{
  CSingleLock lock(m_managerStateMutex);
  return m_managerState;
}

void CWebManager::SetState(CWebManager::ManagerState state)
{
  ObservableMessage observableMsg(ObservableMessageNone);

  {
    CSingleLock lock(m_managerStateMutex);
    if (m_managerState == state)
      return;

    m_managerState = state;

    WebEvent event;
    switch (state)
    {
      case ManagerStateError:
        event = WEB::ManagerError;
        break;
      case ManagerStateStopped:
        event = WEB::ManagerStopped;
        observableMsg = ObservableMessageManagerStopped;
        break;
      case ManagerStateStarting:
        event = WEB::ManagerStarting;
        break;
      case ManagerStateStopping:
        event = WEB::ManagerStopped;
        break;
      case ManagerStateInterrupted:
        event = WEB::ManagerInterrupted;
        break;
      case ManagerStateStarted:
        event = WEB::ManagerStarted;
        break;
      default:
        return;
    }
    m_events.Publish(event);
  }

  if (observableMsg != ObservableMessageNone)
  {
    SetChanged();
    NotifyObservers(observableMsg);
  }
}

void CWebManager::Unload(void)
{
  CSingleLock lock(m_critSection);

  for (const auto& addon : m_addonMap)
  {
    addon.second->Destroy();
  }
  m_addonMap.clear();
}

void CWebManager::UpdateAddons(void)
{
  BinaryAddonBaseList addonInfos;
  CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, false, ADDON_WEB_MANAGER);
  if (addonInfos.empty())
    return;

  for (auto &addon : addonInfos)
  {
    bool enabled = CServiceBroker::GetBinaryAddonManager().IsAddonEnabled(addon->ID(), ADDON_WEB_MANAGER);
    UpdateAddon(addon, enabled);
  }
}

void CWebManager::UpdateAddon(const BinaryAddonBasePtr& addon, bool enabled)
{
  if (enabled && (!IsKnown(addon) || !IsCreatedAddon(addon)))
  {
    std::hash<std::string> hasher;
    int iAddonId = static_cast<int>(hasher(addon->ID()));
    if (iAddonId < 0)
      iAddonId = -iAddonId;

    CWebAddonPtr webAddon;
    ADDON_STATUS status = ADDON_STATUS_UNKNOWN;
    if (IsKnown(addon))
    {
      GetAddon(iAddonId, webAddon);
      status = webAddon->Create(iAddonId);
    }
    else
    {
      webAddon = std::make_shared<CWebAddon>(addon);
      if (!webAddon)
      {
        CLog::Log(LOGERROR, "CWebManager::%s - severe error, incorrect add-on type", __FUNCTION__);
        return;
      }

      status = webAddon.get()->Create(iAddonId);

      // register the add-on
      if (m_addonMap.find(iAddonId) == m_addonMap.end())
      {
        m_addonMap.insert(std::make_pair(iAddonId, webAddon));
        m_addonNameIds.insert(make_pair(addon->ID(), iAddonId));
      }
    }

    if (status != ADDON_STATUS_OK)
    {
      CLog::Log(LOGERROR, "CWebManager::%s - failed to create add-on %s, status = %d", __FUNCTION__, addon->Name().c_str(), status);
      if (status == ADDON_STATUS_PERMANENT_FAILURE)
      {
        CServiceBroker::GetAddonMgr().DisableAddon(addon->ID());
      }
    }
    else if (webAddon)
    {
      if (m_activeWebbrowser == nullptr)
        m_activeWebbrowser = webAddon;
    }
  }
  else if (IsCreatedAddon(addon))
  {
    // stop add-on if it's no longer enabled, restart add-on if it's still enabled
    StopAddon(addon, enabled);
  }
}

int CWebManager::GetAddonId(const std::string& strId) const
{
  CSingleLock lock(m_critSection);

  const auto& citr = m_addonNameIds.find(strId);
  return citr != m_addonNameIds.end() ?
      citr->second :
      -1;
}

int CWebManager::GetAddonId(const BinaryAddonBasePtr& addon) const
{
  CSingleLock lock(m_critSection);

  for (const auto& entry : m_addonMap)
    if (entry.second->ID() == addon->ID())
      return entry.first;

  return -1;
}

bool CWebManager::GetAddon(int addonId, CWebAddonPtr &addon) const
{
  bool bReturn(false);
  if (addonId <= INVALID_WEB_ADDON_ID)
    return bReturn;

  CSingleLock lock(m_critSection);

  const auto& citr = m_addonMap.find(addonId);
  if (citr != m_addonMap.end())
  {
    addon = citr->second;
    bReturn = true;
  }

  return bReturn;
}

bool CWebManager::StopAddon(const BinaryAddonBasePtr& addon, bool bRestart)
{
  CSingleLock lock(m_critSection);
  int iId = GetAddonId(addon);
  CWebAddonPtr mappedAddon;
  if (GetAddon(iId, mappedAddon))
  {
    if (bRestart)
      mappedAddon->ReCreate();
    else
    {
      const auto it = m_addonMap.find(iId);
      if (it != m_addonMap.end())
        m_addonMap.erase(it);

      mappedAddon->Destroy();
    }
    return true;
  }

  return false;
}

bool CWebManager::IsKnown(const BinaryAddonBasePtr& addon) const
{
  // database IDs start at 1
  return GetAddonId(addon) > 0;
}

bool CWebManager::HasCreatedAddons() const
{
  CSingleLock lock(m_critSection);

  for (auto &addon : m_addonMap)
  {
    if (addon.second->ReadyToUse())
    {
      return true;
    }
  }

  return false;
}

bool CWebManager::HasBrowser() const
{
  CSingleLock lock(m_critSection);
  return m_activeWebbrowser != nullptr;
}

bool CWebManager::GetFavourites(const std::string& strPath, CFileItemList& items)
{
  CWebDatabase database;
  if (!database.Open())
    return false;

  std::vector<FavouriteEntryPtr> entries;
  if (!database.GetFavourites(entries))
    return false;

  for (const auto& entry : entries)
  {
    CFileItemPtr pFileItem(new CFileItem());
    pFileItem->SetLabel(entry->GetName());
    pFileItem->SetLabel2(entry->GetURL());
    pFileItem->SetProperty("dateadded", CVariant{entry->GetDateTimeAdded().GetAsLocalizedDateTime(true, false)});
    pFileItem->SetIconImage(entry->GetIcon());
    pFileItem->SetPath("web://sites/favourites/" + entry->GetURL());
    items.Add(pFileItem);
  }

  return true;
}

bool CWebManager::GetRunningWebsites(const std::string& strPath, CFileItemList& items)
{
  for (const auto& control : m_runningGUIControls)
  {
    CFileItemPtr pFileItem(new CFileItem());
    pFileItem->SetLabel(control->OpenedTitle());
    pFileItem->SetLabel2(control->OpenedAddress());
    pFileItem->SetIconImage(control->IconURL());
    pFileItem->SetProperty("browseridstring", CVariant{control->WebControlIdString()});
    pFileItem->SetProperty("width", CVariant{control->GetWidth()});
    pFileItem->SetProperty("height", CVariant{control->GetHeight()});
    pFileItem->SetPath("web://sites/running/" + control->WebControlIdString());
    items.Add(pFileItem);
  }

  return true;
}

void CWebManager::ExecuteItem(CFileItem& item)
{
  std::string url = item.GetPath();
  if (StringUtils::StartsWith(url, "web://sites/favourites/"))
  {
    StringUtils::Replace(url, "web://sites/favourites/", "");

    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_WEB_BROWSER);
    CGUIMessage msg(GUI_MSG_WEB_NEW_TAB, 0, WINDOW_WEB_BROWSER);
    msg.SetLabel(url);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
  else if (StringUtils::StartsWith(url, "web://sites/running/"))
  {
    StringUtils::Replace(url, "web://sites/running/", "");

    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_WEB_BROWSER);
    CGUIMessage msg(GUI_MSG_WEB_OPEN_TAB, 0, WINDOW_WEB_BROWSER);
    msg.SetLabel(url);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
}

void CWebManager::RegisterRunningWebGUIControl(CGUIWebAddonControl* control, int tabId)
{
  m_runningGUIControls.insert(m_runningGUIControls.begin()+tabId, control);
  m_events.Publish(WEB::ManagerChanged);
}

void CWebManager::UnregisterRunningWebGUIControl(CGUIWebAddonControl* control)
{
  for (std::vector<CGUIWebAddonControl*>::iterator it = m_runningGUIControls.begin() ; it != m_runningGUIControls.end(); ++it)
  {
    if (*it == control)
    {
      m_runningGUIControls.erase(it);
      break;
    }
  }
  m_events.Publish(WEB::ManagerChanged);
}

void CWebManager::PublishEvent(WebEvent event)
{
  m_events.Publish(event);
}

//------------------------------------------------------------------------------

void CWebManager::CAddonControl::Clear()
{
  m_runningWebAddonControls.clear();
}

WebAddonControlPtr CWebManager::CAddonControl::GetControl(const std::string id)
{
  const auto& entry = m_runningWebAddonControls.find(id);
  if (entry != m_runningWebAddonControls.end())
    return entry->second->shared_from_this();

  CWebAddonPtr addon = m_manager->GetActiveWebBrowser();
  if (!addon)
    return nullptr;

  WebAddonControlPtr control = std::make_shared<CWebAddonControl>(id, addon);
  RegisterControl(control.get(), id);
  return control;
}

void CWebManager::CAddonControl::RegisterControl(CWebAddonControl* control, const std::string id)
{
  m_runningWebAddonControls.insert(std::pair<std::string, CWebAddonControl*>(id, control));
}

void CWebManager::CAddonControl::UnregisterControl(const std::string id)
{
  m_runningWebAddonControls.erase(id);
}
