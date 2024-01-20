/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/controllers/types/ControllerTree.h"
#include "peripherals/PeripheralTypes.h"

#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace KODI
{
namespace GAME
{
class CAgentController;

/*!
 * \ingroup games
 */
class CAgentControllerMap
{
public:
  CAgentControllerMap();
  ~CAgentControllerMap();

  void Clear();

  void Initialize();
  void Deinitialize();

  // Controller accessors
  std::shared_ptr<const CAgentController> GetAgentController(unsigned int controllerId) const;
  std::vector<std::shared_ptr<const CAgentController>> GetConnectedControllers() const;
  float GetPeripheralActivation(const std::string& peripheralLocation) const;

  // Controller mutators
  std::vector<std::shared_ptr<const CAgentController>> HandlePeripherals(
      const PERIPHERALS::PeripheralVector& peripherals);

  // XML functions
  bool LoadXML();
  void SaveXMLAsync();

private:
  /*!
   * \brief ID -> controller map
   */
  using ControllerIDMap = std::map<unsigned int, std::shared_ptr<CAgentController>>;

  // Controller mutators
  std::shared_ptr<const CAgentController> AddPeripheral(PERIPHERALS::PeripheralPtr peripheral);

  // XML functions
  static bool SerializeControllers(tinyxml2::XMLElement& controllersElement,
                                   const ControllerIDMap& controllers);
  static bool SerializeController(tinyxml2::XMLElement& controllerElement,
                                  const CAgentController& controller);
  static bool DeserializeControllers(const tinyxml2::XMLElement& controllersElement,
                                     ControllerIDMap& controllersById);
  static bool DeserializeController(const tinyxml2::XMLElement& controllerElement,
                                    CAgentController& controller);

  // Filesystem parameters
  std::string m_controllersXmlPath;
  std::vector<std::future<void>> m_saveFutures;
  std::mutex m_saveMutex;

  // Input parameters
  ControllerIDMap m_controllersById;
};
} // namespace GAME
} // namespace KODI
