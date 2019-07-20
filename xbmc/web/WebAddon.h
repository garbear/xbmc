/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Web.h"
#include "addons/binary-addons/AddonInstanceHandler.h"

namespace WEB
{

class CWebAddon;
typedef std::shared_ptr<CWebAddon> CWebAddonPtr;
#define INVALID_WEB_ADDON_ID -1

/*!
  * Interface from Kodi to a Web add-on.
  *
  * Also translates Kodi's C++ structures to the addon's C structures.
  */
class CWebAddon : public ADDON::IAddonInstanceHandler
{
public:
  CWebAddon(ADDON::BinaryAddonBasePtr addonBase);
  ~CWebAddon() override;

  /** @name web add-on methods */
  //@{

  /*!
    * @brief Initialise the instance of this add-on.
    * @param iClientId The ID of this add-on.
    */
  ADDON_STATUS Create(int addonId);

  /*!
    * @brief Destroy the instance of this add-on.
    */
  void Destroy(void);

  /*!
    * @brief Destroy and recreate this add-on.
    */
  void ReCreate(void);

  /*!
    * @return True if this instance is initialised, false otherwise.
    */
  bool ReadyToUse(void) const;

  /*!
    * @return The ID of this instance.
    */
  int GetID(void) const;

  /*!
    * @brief From application main thread started Initialize call
    *
    * @return true if successfull initialized
    */
  bool MainInitialize();

  /*!
    * @brief From application main thread called main loop
    */
  void MainLoop();

  /*!
    * @brief From application main started shutdown call
    */
  void MainShutdown();

  /*!
    * @brief To inform addon about selected audio mute of Kodi
    */
  void SetMute(bool mute);

  /*!
    * @brief To inform addon about selected language of Kodi
    */
  void SetLanguage(std::string language);
  //@}

  /*!
    * GUI control related functions
    */
  //@{
  WEB_ADDON_ERROR ControlCreate(const WEB_ADDON_GUI_PROPS &props, const std::string& startURL, ADDON_HANDLE handle);
  bool DestroyControl(const ADDON_HANDLE handle, bool complete);
  void Render(const ADDON_HANDLE handle);
  bool Dirty(const ADDON_HANDLE handle);
  bool OnInit(const ADDON_HANDLE handle);
  bool OnAction(const ADDON_HANDLE handle, int actionId, uint32_t buttoncode, wchar_t unicode, int& nextItem);
  bool OnMouseEvent(const ADDON_HANDLE handle, int id, double x, double y, double offsetX, double offsetY, int state);
  bool ControlGetHistory(const ADDON_HANDLE handle, std::vector<std::string>& historyWebsiteNames, bool behindCurrent);
  void ControlSearchText(const ADDON_HANDLE handle, const std::string& text, bool forward, bool matchCase, bool findNext);
  void ControlStopSearch(const ADDON_HANDLE handle, bool clearSelection);
  void ControlScreenSizeChange(const ADDON_HANDLE handle, float x, float y, float width, float height, bool fullscreen);
  //@}

  /*!
    * Internet browser related functions
    */
  //@{
  bool OpenWebsite(const ADDON_HANDLE handle, const std::string& url);
  void Reload(const ADDON_HANDLE handle);
  void StopLoad(const ADDON_HANDLE handle);
  void GoBack(const ADDON_HANDLE handle);
  void GoForward(const ADDON_HANDLE handle);
  void OpenOwnContextMenu(const ADDON_HANDLE handle);
  //@}

private:
  /*!
    * @brief Callback functions from addon to kodi
    */
  //@{
  static bool cb_is_muted(void* kodiInstance);
  static void cb_control_set_control_ready(void* kodiInstance, void* handle, bool ready);
  static void cb_control_set_opened_address(void* kodiInstance, void* handle, const char* address);
  static void cb_control_set_opened_title(void* kodiInstance, void* handle, const char* title);
  static void cb_control_set_icon_url(void* kodiInstance, void* handle, const char* icon);
  static void cb_control_set_fullscreen(void* kodiInstance, void* handle, bool fullscreen);
  static void cb_control_set_loading_state(void* kodiInstance, void* handle, bool  isLoading, bool canGoBack, bool canGoForward);
  static void cb_control_set_tooltip(void* kodiInstance, void* handle, const char* tooltip);
  static void cb_control_set_status_message(void* kodiInstance, void* handle, const char* status);
  static void cb_control_request_open_site_in_new_tab(void* kodiInstance, void* handle, const char* url);
  //@}

  static const char *ToString(const WEB_ADDON_ERROR error);
  bool LogError(const WEB_ADDON_ERROR error, const char *strMethod) const;
  void ResetProperties(int addonId = -1);

  bool m_readyToUse;                /*!< true if this add-on is connected to the backend, false otherwise */
  bool m_readyToProcess = false;
  bool m_initCalled = false;
  int m_addonId;                    /*!< database ID of the add-on */

  CCriticalSection m_critSection;   /*!< */

  /* stored strings to make sure const char* members in WEB_ADDON_PROPERTIES stay valid */
  std::string m_strUserPath;        /*!< translated path to the user profile */
  std::string m_strAddonLibPath;    /*!< translated path to this add-on */
  std::string m_strAddonSharePath;  /*!< translated path to this add-on */

  AddonInstance_WebAddon m_struct;
};

} /* namespace WEB */
