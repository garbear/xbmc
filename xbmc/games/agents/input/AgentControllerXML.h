/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

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
class CAgentControllerXML
{
public:
  // Serialization helper functions
  static bool SerializeController(tinyxml2::XMLElement& controllerElement,
                                  const CAgentController& controller);
  static bool SerializeControllerProperties(tinyxml2::XMLElement& controllerElement,
                                            const CAgentController& controller);
  static bool SerializePeripheralProperties(tinyxml2::XMLElement& controllerElement,
                                            const CAgentController& controller);
  static bool SerializeJoystickProperties(tinyxml2::XMLElement& controllerElement,
                                          const CAgentController& controller);
  static bool SerializeConfig(tinyxml2::XMLElement& controllerElement,
                              const CAgentController& controller);
  static bool SerializeLastActive(tinyxml2::XMLElement& controllerElement,
                                  const CAgentController& controller);

  // Deserialization helper functions
  static bool DeserializeController(const tinyxml2::XMLElement& controllerElement,
                                    CAgentController& controller);
  static bool DeserializeControllerProperties(const tinyxml2::XMLElement& controllerElement,
                                              CAgentController& controller);
  static bool DeserializePeripheralProperties(const tinyxml2::XMLElement& controllerElement,
                                              CAgentController& controller);
  static bool DeserializeJoystickProperties(const tinyxml2::XMLElement& controllerElement,
                                            CAgentController& controller);
  static bool DeserializeConfig(const tinyxml2::XMLElement& controllerElement,
                                CAgentController& controller);
  static bool DeserializeLastActive(const tinyxml2::XMLElement& controllerElement,
                                    CAgentController& controller);
};
} // namespace GAME
} // namespace KODI
