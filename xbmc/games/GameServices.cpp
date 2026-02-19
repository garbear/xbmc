/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameServices.h"

#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "games/GameSettings.h"
#include "games/GameUtils.h"
#include "games/agents/input/AgentInput.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessageIDs.h"
#include "guilib/GUIWindowManager.h"
#include "profiles/ProfileManager.h"
#include "threads/Event.h"

using namespace KODI;
using namespace GAME;

namespace
{
void InstallAddon(const std::string& controllerId)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // If the addon isn't installed we need to install it
  bool installed = addonManager.IsAddonInstalled(controllerId);
  if (!installed)
  {
    installed = ADDON::CAddonInstaller::GetInstance().InstallOrUpdate(
        controllerId, ADDON::BackgroundJob::CHOICE_YES, ADDON::ModalJob::CHOICE_NO);
  }

  if (installed)
  {
    // Make sure add-on is enabled
    if (addonManager.IsAddonDisabled(controllerId))
      addonManager.EnableAddon(controllerId);
  }
}
} // namespace

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager,
                             CInputManager& inputManager,
                             ADDON::CAddonMgr& addons)
  : m_controllerManager(controllerManager),
    m_gameRenderManager(renderManager),
    m_profileManager(profileManager),
    m_gameSettings(new CGameSettings()),
    m_agentInput(std::make_unique<CAgentInput>(peripheralManager, inputManager)),
    m_videoShaders(std::make_unique<SHADER::CShaderPresetFactory>(addons)),
    m_guiReadyEvent(std::make_unique<CEvent>())
{
  // Load the add-ons from the database asynchronously
  m_initializationTask =
      std::async(std::launch::async, []() { CGameUtils::UpdateInstallableAddons(); });
}

CGameServices::~CGameServices()
{
  m_guiReadyEvent->Set();
}

void CGameServices::Initialize()
{
  // Must not call this from the constructor because the controller tree
  // calls back into CGameServices to get controller profiles
  m_agentInput->Initialize();

  // Register for GUI messages
  if (CGUIComponent* gui = CServiceBroker::GetGUI())
    gui->GetWindowManager().AddMsgTarget(this);

  // Install missing controller profiles on startup
  InstallControllerProfiles();
}

void CGameServices::Deinitialize()
{
  // Unregister with GUI messenger
  if (CGUIComponent* gui = CServiceBroker::GetGUI())
    gui->GetWindowManager().RemoveMsgTarget(this);

  m_guiReadyEvent->Set();

  m_agentInput->Deinitialize();
}

ControllerPtr CGameServices::GetController(const std::string& controllerId)
{
  return m_controllerManager.GetController(controllerId);
}

ControllerPtr CGameServices::GetDefaultController()
{
  return m_controllerManager.GetDefaultController();
}

ControllerPtr CGameServices::GetDefaultKeyboard()
{
  return m_controllerManager.GetDefaultKeyboard();
}

ControllerPtr CGameServices::GetDefaultMouse()
{
  return m_controllerManager.GetDefaultMouse();
}

ControllerVector CGameServices::GetControllers()
{
  return m_controllerManager.GetControllers();
}

std::string CGameServices::TranslateFeature(const std::string& controllerId,
                                            const std::string& featureName)
{
  return m_controllerManager.TranslateFeature(controllerId, featureName);
}

std::string CGameServices::GetSavestatesFolder() const
{
  return m_profileManager.GetSavestatesFolder();
}

void CGameServices::OnAddonRepoInstalled()
{
  CGameUtils::UpdateInstallableAddons();
}

bool CGameServices::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UI_READY)
      {
        m_guiReady = true;
        m_guiReadyEvent->Set();
      }
      break;
    }
    default:
      break;
  }

  return false;
}

bool CGameServices::WaitForGUI()
{
  m_guiReadyEvent->Wait();
  return m_guiReady;
}

void CGameServices::InstallControllerProfiles()
{
  std::vector<std::string> controllerIds{"game.controller.default"};

  for (const std::string& controllerId : controllerIds)
  {
    m_controllerInstallTasks.emplace_back(std::async(std::launch::async,
                                                     [this, controllerId]()
                                                     {
                                                       if (!WaitForGUI())
                                                         return;

                                                       std::unique_lock lockInstall{
                                                           m_controllerInstallMutex};

                                                       InstallAddon(controllerId);
                                                     }));
  }
}
