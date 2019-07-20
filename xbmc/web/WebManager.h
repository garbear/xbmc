/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebAddon.h"
#include "WebAddonControl.h"
#include "WebEvent.h"

#include "Application.h"
#include "GUIInfoManager.h"
#include "addons/AddonDatabase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Web.h"
#include "messaging/IMessageTarget.h"
#include "settings/lib/ISettingCallback.h"

#include <vector>
#include <map>
#include <deque>

class CStopWatch;

#define WEB_ADDON_STATE_ON        0
#define WEB_ADDON_STATE_OFF       1
#define WEB_ADDON_SYNC_ACTIVATE   0
#define WEB_ADDON_ASYNC_ACTIVATE  1

namespace WEB
{

class CWebDatabase;
class CWebGUIInfo;
typedef std::shared_ptr<CWebDatabase> CWebDatabasePtr;

class CGUIWebAddonControl;
class CWebDatabase;
typedef std::map< int, CWebAddonPtr >                 WEB_ADDON_MAP;
typedef std::map< int, CWebAddonPtr >::iterator       WEB_ADDON_MAP_ITR;
typedef std::map< int, CWebAddonPtr >::const_iterator WEB_ADDON_MAP_CITR;

class CWebManager
  : public Observable
{
public:
  CWebManager();
  ~CWebManager() override;

  void Init(bool thisThread = false);
  void Stop();
  void Deinit();

  bool IsStarted() { return m_started; }

  bool MainInitialize();
  void MainLoop();
  void MainShutdown();

  void SetMute(bool muted);












  bool IsInUse(const std::string& strAddonId) const;



  bool HasCreatedAddons() const;
  bool HasBrowser() const;


  bool IsWebWindowActive(void) const;
  static bool IsWebWindow(int windowId);
  void LocalizationChanged(std::string strLanguage);



bool IsCreatedAddon(const ADDON::BinaryAddonBasePtr& addon);

private:
  void Clear(void);

  CEvent                     m_triggerEvent;                /*!< triggers an update */


protected:

  mutable CCriticalSection            m_critSection;              /*!< Critical lock for control functions */

public:

  virtual void EnableEvent(ADDON::BinaryAddonBasePtr addon);// override;
  virtual void DisableEvent(ADDON::BinaryAddonBasePtr addon);// override;
  virtual void UpdateEvent(ADDON::BinaryAddonBasePtr addon);// override;
  virtual void InstallEvent(ADDON::BinaryAddonBasePtr addon, bool update, bool modal);// override;
  virtual void PreUnInstallEvent(ADDON::BinaryAddonBasePtr addon);// override;

  inline bool IsInitialising(void) const { return GetState() == ManagerStateStarting; }
  inline bool IsStarted(void) const { return GetState() == ManagerStateStarted; }
  inline bool IsStopping(void) const { return GetState() == ManagerStateStopping; }
  inline bool IsStopped(void) const { return GetState() == ManagerStateStopped; }

  bool GetFavourites(const std::string& strPath, CFileItemList& items);
  bool GetRunningWebsites(const std::string& strPath, CFileItemList& items);
  void ExecuteItem(CFileItem& item);
  const std::vector<CGUIWebAddonControl*>& GetRunningGUIControls() { return m_runningGUIControls; }

  const CWebAddonPtr& GetActiveWebBrowser() { return m_activeWebbrowser; }

  void RegisterRunningWebGUIControl(CGUIWebAddonControl* control, int tabId);
  void UnregisterRunningWebGUIControl(CGUIWebAddonControl* control);

  void PublishEvent(WebEvent state);
  CEventStream<WebEvent>& Events() { return m_events; }

private:
  enum ManagerState
  {
    ManagerStateError = 0,
    ManagerStateStopped,
    ManagerStateStarting,
    ManagerStateStopping,
    ManagerStateInterrupted,
    ManagerStateStarted
  };

  ManagerState GetState(void) const;
  void SetState(ManagerState state);

  void Unload(void);
  void UpdateAddons(void);
  void UpdateAddon(const ADDON::BinaryAddonBasePtr& addon, bool enabled);
  int GetAddonId(const ADDON::BinaryAddonBasePtr& addon) const;
  void UnRegisterAddon(const std::string& addonId);
  int GetAddonId(const std::string& addonId) const;
  bool GetAddon(int iAddonId, CWebAddonPtr &addon) const;
  bool StopAddon(const ADDON::BinaryAddonBasePtr& addon, bool bRestart);
  bool IsKnown(const ADDON::BinaryAddonBasePtr& addon) const;

  bool m_started = false;

  mutable CCriticalSection m_managerStateMutex;
  ManagerState m_managerState;

  std::map<std::string, int> m_addonNameIds;
  CEventSource<WebEvent> m_events;
  std::vector<CGUIWebAddonControl*> m_runningGUIControls;

  WEB_ADDON_MAP m_addonMap;
  CWebAddonPtr m_activeWebbrowser;

public:
  /*!
    * To handle controls on addon itself
    *
    * Kodi's web gui control for the same part can be used on several
    * places. Every of them use then the single defined on addon from
    * here.
    *
    * A control on addon becomes identified by a string, with this
    * can be the same used on different places.
    */
  class CAddonControl
  {
  public:
    CAddonControl(CWebManager* manager) : m_manager(manager) { }

    WebAddonControlPtr GetControl(const std::string id);
    void Clear();

  private:
    std::map<std::string, CWebAddonControl*> m_runningWebAddonControls;
    CWebManager* m_manager;

    void RegisterControl(CWebAddonControl* control, const std::string id);
    friend class CWebAddonControl; /* Have UnregisterWebAddonControl(...) as friend part for CWebAddonControl */
    void UnregisterControl(const std::string id);
  } m_addonControl;
};

} /* namespace WEB */
