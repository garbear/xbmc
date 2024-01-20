/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentControllerMap.h"

#include "AgentController.h"
#include "URL.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto PROFILE_ROOT = "special://masterprofile";

constexpr auto CONTROLLERS_XML_FILE = "gamecontrollers.xml"; //! @todo Move to "games" subfolder

constexpr auto XML_ELM_CONTROLLERS = "controllers";
constexpr auto XML_ELM_CONTROLLER = "controller";
} // namespace

CAgentControllerMap::CAgentControllerMap()
{
  Initialize();
}

CAgentControllerMap::~CAgentControllerMap()
{
  Deinitialize();
}

void CAgentControllerMap::Clear()
{
  // Clear input parameters
  m_controllersById.clear();
};

void CAgentControllerMap::Initialize()
{
  m_controllersXmlPath = URIUtils::AddFileToFolder(PROFILE_ROOT, CONTROLLERS_XML_FILE);
}

void CAgentControllerMap::Deinitialize()
{
  // Wait for save tasks
  for (std::future<void>& task : m_saveFutures)
    task.wait();
  m_saveFutures.clear();

  // Reset state
  Clear();
}

bool CAgentControllerMap::LoadXML()
{
  Clear();

  // Check if controllers.xml file exists
  if (!CFileUtils::Exists(m_controllersXmlPath))
  {
    CLog::Log(LOGDEBUG, "Can't load controllers, file doesn't exist: \"{}\"",
              CURL::GetRedacted(m_controllersXmlPath));
    return false;
  }

  CLog::Log(LOGINFO, "Loading controllers: {}", CURL::GetRedacted(m_controllersXmlPath));

  // Load controllers
  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(m_controllersXmlPath))
  {
    CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorStr(),
              xmlDoc.ErrorLineNum());
    return false;
  }

  // Validate root element
  const auto* rootElement = xmlDoc.RootElement();
  if (rootElement == nullptr || std::strcmp(rootElement->Value(), XML_ELM_CONTROLLERS) != 0)
  {
    CLog::Log(LOGERROR, "Can't find root <{}> tag", XML_ELM_CONTROLLERS);
    return false;
  }

  // Deserialize controllers
  if (!DeserializeControllers(*rootElement, m_controllersById))
    return false;

  return true;
}

void CAgentControllerMap::SaveXMLAsync()
{
  ControllerIDMap controllers = m_controllersById;

  // Prune any finished save tasks
  m_saveFutures.erase(std::remove_if(m_saveFutures.begin(), m_saveFutures.end(),
                                     [](std::future<void>& task) {
                                       return task.wait_for(std::chrono::seconds(0)) ==
                                              std::future_status::ready;
                                     }),
                      m_saveFutures.end());

  // Save async
  std::future<void> task = std::async(std::launch::async,
                                      [this, controllers = std::move(controllers)]()
                                      {
                                        CXBMCTinyXML2 doc;
                                        auto* rootElement = doc.NewElement(XML_ELM_CONTROLLERS);
                                        if (rootElement == nullptr)
                                          return;

                                        CLog::Log(LOGDEBUG, "Saving controllers to {}",
                                                  CURL::GetRedacted(m_controllersXmlPath));

                                        if (SerializeControllers(*rootElement, controllers))
                                        {
                                          doc.InsertEndChild(rootElement);

                                          std::lock_guard<std::mutex> lock(m_saveMutex);
                                          doc.SaveFile(m_controllersXmlPath);
                                        }
                                      });

  m_saveFutures.emplace_back(std::move(task));
}

std::shared_ptr<const CAgentController> CAgentControllerMap::GetAgentController(
    unsigned int controllerId) const
{
  const auto it = m_controllersById.find(controllerId);
  if (it != m_controllersById.end())
    return std::const_pointer_cast<const CAgentController>(it->second);

  return {};
}

std::vector<std::shared_ptr<const CAgentController>> CAgentControllerMap::GetConnectedControllers()
    const
{
  std::vector<std::shared_ptr<const CAgentController>> controllers;

  for (const auto& controller : m_controllersById)
  {
    // Dereference iterator
    const CAgentController& agentController = *controller.second;

    // Check if controller is connected
    if (!agentController.IsConnected())
      continue;

    controllers.emplace_back(std::const_pointer_cast<const CAgentController>(controller.second));
  }

  return controllers;
}

float CAgentControllerMap::GetPeripheralActivation(const std::string& peripheralLocation) const
{
  for (const auto& controller : m_controllersById)
  {
    // Dereference iterator
    const CAgentController& agentController = *controller.second;

    // Check if controller location matches
    if (agentController.GetPeripheralLocation() == peripheralLocation)
      return agentController.GetActivation();
  }

  return 0.0f;
}

std::vector<std::shared_ptr<const CAgentController>> CAgentControllerMap::HandlePeripherals(
    const PERIPHERALS::PeripheralVector& peripherals)
{
  std::vector<std::shared_ptr<const CAgentController>> connectedControllers;

  // Handle new and existing peripherals
  for (const auto& peripheral : peripherals)
  {
    // Add peripheral to controller map
    std::shared_ptr<const CAgentController> agentController = AddPeripheral(peripheral);
    connectedControllers.emplace_back(std::move(agentController));
  }

  // Remove expired peripherals
  std::vector<std::string> expiredPeripherals;
  for (const auto& agentControllerIt : m_controllersById)
  {
    // Dereference iterator
    std::shared_ptr<const CAgentController> agentController =
        std::const_pointer_cast<const CAgentController>(agentControllerIt.second);

    auto it =
        std::find_if(peripherals.begin(), peripherals.end(),
                     [&agentController](const PERIPHERALS::PeripheralPtr& peripheral) {
                       return agentController->GetPeripheralLocation() == peripheral->Location();
                     });

    if (it == peripherals.end())
      expiredPeripherals.emplace_back(agentController->GetPeripheralLocation());
  }
  for (const std::string& expiredJoystick : expiredPeripherals)
  {
    auto it = std::find_if(m_controllersById.begin(), m_controllersById.end(),
                           [&expiredJoystick](const auto& controllerId) {
                             return controllerId.second->GetPeripheralLocation() == expiredJoystick;
                           });
    if (it != m_controllersById.end())
    {
      // Dereference iterator
      CAgentController& agentController = *it->second;

      //! @todo
      /*
      if (!inputHandlingLock)
        inputHandlingLock = m_peripheralManager.RegisterEventLock();
      */

      // Deinitialize agent
      agentController.Deinitialize();

      // Detach input
      agentController.DetachPeripheral();

      // Remove from list
      m_controllersById.erase(it);

      SaveXMLAsync();
    }
  }

  return connectedControllers;
}

std::shared_ptr<const CAgentController> CAgentControllerMap::AddPeripheral(
    PERIPHERALS::PeripheralPtr peripheral)
{
  std::shared_ptr<CAgentController> agentController;

  CLog::Log(LOGINFO, "Adding peripheral at location {}", peripheral->Location());

  // Re-use existing controller if possible
  //! @todo More robust check
  auto it = std::find_if(m_controllersById.begin(), m_controllersById.end(),
                         [&peripheral](const auto& controllerIt)
                         {
                           // Dereference iterator
                           const CAgentController& agentController = *controllerIt.second;

                           // Check if controller location matches
                           return agentController.GetPeripheralLocation() == peripheral->Location();
                         });

  if (it == m_controllersById.end())
  {
    // Create agent controller
    agentController = std::make_shared<CAgentController>(peripheral);

    // Calculate next controller ID
    unsigned int nextControllerId = 0;
    if (!m_controllersById.empty())
      nextControllerId = m_controllersById.rbegin()->first + 1;

    // Set controller ID
    agentController->SetControllerID(nextControllerId);

    // Add controller
    m_controllersById[nextControllerId] = agentController;

    // Save controllers
    SaveXMLAsync();

    return agentController;
  }
  else
  {
    // Check if appearance has changed
    agentController = it->second;

    ControllerPtr oldController = agentController->GetController();
    ControllerPtr newController = peripheral->ControllerProfile();

    std::string oldControllerId = oldController ? oldController->ID() : "";
    std::string newControllerId = newController ? newController->ID() : "";

    if (oldControllerId != newControllerId)
    {
      //! @todo
      /*
      auto inputHandlingLock = m_peripheralManager.RegisterEventLock();
      */

      // Reinitialize agent's controller
      agentController->Deinitialize();
      agentController->Initialize();

      // Save controllers
      SaveXMLAsync();
    }
  }

  return std::const_pointer_cast<const CAgentController>(agentController);
}

bool CAgentControllerMap::SerializeControllers(tinyxml2::XMLElement& node,
                                               const ControllerIDMap& controllers)
{
  // Iterate over all controllers
  for (const auto& controller : controllers)
  {
    // Dereference iterator
    const CAgentController& agentController = *controller.second;

    // Create controller element
    tinyxml2::XMLElement* controllerElement = node.GetDocument()->NewElement(XML_ELM_CONTROLLER);
    if (controllerElement == nullptr)
      return false;

    // Serialize controller
    if (!SerializeController(*controllerElement, agentController))
      continue;

    // Add controller element
    node.InsertEndChild(controllerElement);
  }

  return true;
}

bool CAgentControllerMap::SerializeController(tinyxml2::XMLElement& controllerElement,
                                              const CAgentController& controller)
{
  return controller.Serialize(controllerElement);
}

bool CAgentControllerMap::DeserializeControllers(const tinyxml2::XMLElement& controllersElement,
                                                 ControllerIDMap& controllersById)
{
  // Get first "controller" element
  const tinyxml2::XMLElement* controllerElement =
      controllersElement.FirstChildElement(XML_ELM_CONTROLLER);
  if (controllerElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}>", XML_ELM_CONTROLLER);
    return false;
  }

  // Iterate over all "controller" elements
  while (controllerElement != nullptr)
  {
    // Create controller
    std::shared_ptr<CAgentController> controller = std::make_shared<CAgentController>();
    if (DeserializeController(*controllerElement, *controller))
    {
      // Check controller ID
      if (controllersById.find(controller->GetControllerID()) != controllersById.end())
        CLog::Log(LOGERROR, "Duplicate controller ID with ID {}", controller->GetControllerID());
      else
      {
        // Add controller
        controllersById[controller->GetControllerID()] = std::move(controller);
      }
    }

    // Get next "controller" element
    controllerElement = controllerElement->NextSiblingElement(XML_ELM_CONTROLLER);
  }

  return true;
}

bool CAgentControllerMap::DeserializeController(const tinyxml2::XMLElement& controllerElement,
                                                CAgentController& controller)
{
  return controller.Deserialize(controllerElement);
}
