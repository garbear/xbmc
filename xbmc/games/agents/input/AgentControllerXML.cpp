/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AgentControllerXML.h"

#include "AgentController.h"
#include "AgentJoystick.h"
#include "AgentKeyboard.h"
#include "AgentMouse.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdio>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ELM_CONFIG = "config";
constexpr auto XML_ELM_LAST_ACTIVE = "lastactive";

constexpr auto XML_ATTR_CONTROLLER_ID = "id";
constexpr auto XML_ATTR_PERIPHERAL_TYPE = "type";
constexpr auto XML_ATTR_PERIPHERAL_BUS = "bus";
constexpr auto XML_ATTR_PERIPHERAL_VID = "vid";
constexpr auto XML_ATTR_PERIPHERAL_PID = "pid";
constexpr auto XML_ATTR_PERIPHERAL_LAST_LOCATION = "lastlocation";
constexpr auto XML_ATTR_PERIPHERAL_ORDINAL = "ordinal";

constexpr auto XML_ATTR_JOYSTICK_NAME = "name";
constexpr auto XML_ATTR_JOYSTICK_PROVIDER = "provider";
constexpr auto XML_ATTR_JOYSTICK_BUTTONS = "buttons";
constexpr auto XML_ATTR_JOYSTICK_HATS = "hats";
constexpr auto XML_ATTR_JOYSTICK_AXES = "axes";
constexpr auto XML_ATTR_JOYSTICK_REQUESTED_PORT = "requestedport";

constexpr auto XML_ATTR_CONFIG_APPEARANCE = "apperance";
constexpr auto XML_ATTR_CONFIG_LEFT_DEADZONE = "leftdeadzone";
constexpr auto XML_ATTR_CONFIG_RIGHT_DEADZONE = "rightdeadzone";
} // namespace

bool CAgentControllerXML::SerializeController(tinyxml2::XMLElement& controllerElement,
                                              const CAgentController& controller)
{
  // Set controller properties
  if (!SerializeControllerProperties(controllerElement, controller))
    return false;

  // Set peripheral properties
  if (!SerializePeripheralProperties(controllerElement, controller))
    return false;

  // Set joystick properties
  if (!SerializeJoystickProperties(controllerElement, controller))
    return false;

  // Serialize config element
  if (!SerializeConfig(controllerElement, controller))
    return false;

  // Serialize last active element
  if (!SerializeLastActive(controllerElement, controller))
    return false;

  return true;
}

bool CAgentControllerXML::SerializeControllerProperties(tinyxml2::XMLElement& controllerElement,
                                                        const CAgentController& controller)
{
  // Controller ID
  controllerElement.SetAttribute(XML_ATTR_CONTROLLER_ID, controller.GetControllerID());

  return true;
}

bool CAgentControllerXML::SerializePeripheralProperties(tinyxml2::XMLElement& controllerElement,
                                                        const CAgentController& controller)
{
  // Peripheral type
  controllerElement.SetAttribute(
      XML_ATTR_PERIPHERAL_TYPE,
      PERIPHERALS::PeripheralTypeTranslator::TypeToString(controller.GetPeripheralType()));

  // Peripheral bus
  controllerElement.SetAttribute(
      XML_ATTR_PERIPHERAL_BUS,
      PERIPHERALS::PeripheralTypeTranslator::BusTypeToString(controller.GetPeripheralBus()));

  // USB VID/PID
  if (controller.GetPeripheralVIDHex() != "0000" || controller.GetPeripheralPIDHex() != "0000")
  {
    controllerElement.SetAttribute(XML_ATTR_PERIPHERAL_VID,
                                   controller.GetPeripheralVIDHex().c_str());
    controllerElement.SetAttribute(XML_ATTR_PERIPHERAL_PID,
                                   controller.GetPeripheralPIDHex().c_str());
  }

  // Last location
  controllerElement.SetAttribute(XML_ATTR_PERIPHERAL_LAST_LOCATION,
                                 controller.GetPeripheralLastLocation().c_str());

  // Ordinal
  if (controller.GetPeripheralOrdinal() != 0)
    controllerElement.SetAttribute(XML_ATTR_PERIPHERAL_ORDINAL, controller.GetPeripheralOrdinal());

  return true;
}

bool CAgentControllerXML::SerializeJoystickProperties(tinyxml2::XMLElement& controllerElement,
                                                      const CAgentController& controller)
{
  if (!controller.GetJoystickName().empty())
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_NAME, controller.GetJoystickName().c_str());
  if (!controller.GetJoystickProvider().empty())
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_PROVIDER,
                                   controller.GetJoystickProvider().c_str());
  if (controller.GetJoystickButtons() != 0)
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_BUTTONS, controller.GetJoystickButtons());
  if (controller.GetJoystickHats() != 0)
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_HATS, controller.GetJoystickHats());
  if (controller.GetJoystickAxes() != 0)
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_AXES, controller.GetJoystickAxes());
  if (controller.GetJoystickRequestedPort() != PERIPHERALS::JOYSTICK_NO_PORT_REQUESTED)
    controllerElement.SetAttribute(XML_ATTR_JOYSTICK_REQUESTED_PORT,
                                   controller.GetJoystickRequestedPort());

  return true;
}

bool CAgentControllerXML::SerializeConfig(tinyxml2::XMLElement& controllerElement,
                                          const CAgentController& controller)
{
  // Create config element
  tinyxml2::XMLElement* configElement = controllerElement.GetDocument()->NewElement(XML_ELM_CONFIG);
  if (configElement == nullptr)
    return false;

  // Set config attributes
  configElement->SetAttribute(XML_ATTR_CONFIG_APPEARANCE,
                              controller.GetControllerAppearance().c_str());
  if (controller.GetPeripheralType() == PERIPHERALS::PeripheralType::PERIPHERAL_JOYSTICK)
  {
    configElement->SetAttribute(XML_ATTR_CONFIG_LEFT_DEADZONE, controller.GetLeftDeadzone());
    configElement->SetAttribute(XML_ATTR_CONFIG_RIGHT_DEADZONE, controller.GetRightDeadzone());
  }

  // Add config element to controller element
  controllerElement.InsertEndChild(configElement);

  return true;
}

bool CAgentControllerXML::SerializeLastActive(tinyxml2::XMLElement& controllerElement,
                                              const CAgentController& controller)
{
  std::string strLastActive;

  // Get last active text
  //! @todo
  /*
  if (m_peripheral)
  {
    CDateTime lastActive = m_peripheral->LastActive();
    if (lastActive.IsValid())
      strLastActive = lastActive.GetAsW3CDateTime(true);
  }
  else if (m_lastActiveUtc.IsValid())
    strLastActive = m_lastActiveUtc.GetAsW3CDateTime(true);

  if (!strLastActive.empty())
  {
    // Create last active element
    tinyxml2::XMLElement* lastActiveElement =
        controllerElement.GetDocument()->NewElement(XML_ELM_LAST_ACTIVE);
    if (lastActiveElement == nullptr)
      return false;

    // Set last active text
    lastActiveElement->SetText(strLastActive.c_str());

    // Add last active element to controller element
    controllerElement.InsertEndChild(lastActiveElement);
  }
  */

  return true;
}

bool CAgentControllerXML::DeserializeController(const tinyxml2::XMLElement& controllerElement,
                                                CAgentController& controller)
{
  controller.ClearProperties();

  if (!DeserializeControllerProperties(controllerElement, controller))
    return false;

  if (!DeserializePeripheralProperties(controllerElement, controller))
    return false;

  if (!DeserializeJoystickProperties(controllerElement, controller))
    return false;

  if (!DeserializeConfig(controllerElement, controller))
    return false;

  if (!DeserializeLastActive(controllerElement, controller))
    return false;

  return true;
}

bool CAgentControllerXML::DeserializeControllerProperties(
    const tinyxml2::XMLElement& controllerElement, CAgentController& controller)
{
  // Get controller ID
  const char* controllerId = controllerElement.Attribute(XML_ATTR_CONTROLLER_ID);
  if (controllerId == nullptr)
  {
    CLog::Log(LOGERROR, "Missing attribute \"{}\" in element <{}>", XML_ATTR_CONTROLLER_ID,
              controllerElement.Name());
    return false;
  }

  const int signedControllerId = std::stoi(controllerId);
  if (signedControllerId < 0)
  {
    CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_CONTROLLER_ID, controllerId);
    return false;
  }

  controller.SetControllerID(static_cast<unsigned int>(signedControllerId));

  return true;
}

bool CAgentControllerXML::DeserializePeripheralProperties(
    const tinyxml2::XMLElement& controllerElement, CAgentController& controller)
{
  // Get peripheral type
  const char* peripheralType = controllerElement.Attribute(XML_ATTR_PERIPHERAL_TYPE);
  if (peripheralType == nullptr)
  {
    CLog::Log(LOGERROR, "Missing attribute \"{}\" in element <{}>", XML_ATTR_PERIPHERAL_TYPE,
              controllerElement.Name());
    return false;
  }

  PERIPHERALS::PeripheralType type =
      PERIPHERALS::PeripheralTypeTranslator::GetTypeFromString(peripheralType);
  if (type == PERIPHERALS::PeripheralType::PERIPHERAL_UNKNOWN)
  {
    CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_PERIPHERAL_TYPE,
              peripheralType);
    return false;
  }
  controller.SetPeripheralType(type);

  // Get peripheral bus
  const char* peripheralBus = controllerElement.Attribute(XML_ATTR_PERIPHERAL_BUS);
  if (peripheralBus == nullptr)
  {
    CLog::Log(LOGERROR, "Missing attribute \"{}\" in element <{}>", XML_ATTR_PERIPHERAL_BUS,
              controllerElement.Name());
    return false;
  }

  PERIPHERALS::PeripheralBusType peripheralBusType =
      PERIPHERALS::PeripheralTypeTranslator::GetBusTypeFromString(peripheralBus);
  if (peripheralBusType == PERIPHERALS::PERIPHERAL_BUS_UNKNOWN)
  {
    CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_PERIPHERAL_BUS, peripheralBus);
    return false;
  }
  controller.SetPeripheralBus(peripheralBusType);

  // Get peripheral VID
  const char* peripheralVidHex = controllerElement.Attribute(XML_ATTR_PERIPHERAL_VID);
  if (peripheralVidHex != nullptr)
    controller.SetPeripheralPIDHex(peripheralVidHex);

  // Get peripheral PID
  const char* peripheralPidHex = controllerElement.Attribute(XML_ATTR_PERIPHERAL_PID);
  if (peripheralPidHex != nullptr)
    controller.SetPeripheralPIDHex(peripheralPidHex);

  // Get peripheral last location
  const char* peripheralLastLocation =
      controllerElement.Attribute(XML_ATTR_PERIPHERAL_LAST_LOCATION);
  if (peripheralLastLocation == nullptr)
  {
    CLog::Log(LOGERROR, "Missing attribute \"{}\" in element <{}>",
              XML_ATTR_PERIPHERAL_LAST_LOCATION, controllerElement.Name());
    return false;
  }

  controller.SetPeripheralLastLocation(peripheralLastLocation);

  // Get peripheral ordinal
  const char* peripheralOrdinal = controllerElement.Attribute(XML_ATTR_PERIPHERAL_ORDINAL);
  if (peripheralOrdinal != nullptr)
  {
    const int signedPeripheralOrdinal = std::stoi(peripheralOrdinal);
    if (signedPeripheralOrdinal < 0)
    {
      CLog::Log(LOGERROR, "Invalid attribute \"{}\": \"{}\"", XML_ATTR_PERIPHERAL_ORDINAL,
                peripheralOrdinal);
      return false;
    }

    controller.SetPeripheralOrdinal(static_cast<unsigned int>(signedPeripheralOrdinal));
  }

  return true;
}

bool CAgentControllerXML::DeserializeJoystickProperties(
    const tinyxml2::XMLElement& controllerElement, CAgentController& controller)
{
  // Get joystick name
  const char* joystickName = controllerElement.Attribute(XML_ATTR_JOYSTICK_NAME);
  if (joystickName != nullptr)
    controller.SetJoystickName(joystickName);

  // Get joystick provider
  const char* joystickProvider = controllerElement.Attribute(XML_ATTR_JOYSTICK_PROVIDER);
  if (joystickProvider != nullptr)
    controller.SetJoystickProvider(joystickProvider);

  // Get joystick buttons
  const char* joystickButtons = controllerElement.Attribute(XML_ATTR_JOYSTICK_BUTTONS);
  if (joystickButtons != nullptr)
    controller.SetJoystickButtons(std::atoi(joystickButtons));

  // Get joystick hats
  const char* joystickHats = controllerElement.Attribute(XML_ATTR_JOYSTICK_HATS);
  if (joystickHats != nullptr)
    controller.SetJoystickHats(std::atoi(joystickHats));

  // Get joystick axes
  const char* joystickAxes = controllerElement.Attribute(XML_ATTR_JOYSTICK_AXES);
  if (joystickAxes != nullptr)
    controller.SetJoystickAxes(std::atoi(joystickAxes));

  // Get joystick requested port
  const char* joystickRequestedPort = controllerElement.Attribute(XML_ATTR_JOYSTICK_REQUESTED_PORT);
  if (joystickRequestedPort != nullptr)
    controller.SetJoystickRequestedPort(std::atoi(joystickRequestedPort));

  return true;
}

bool CAgentControllerXML::DeserializeConfig(const tinyxml2::XMLElement& controllerElement,
                                            CAgentController& controller)
{
  // Get config element
  const tinyxml2::XMLElement* configElement = controllerElement.FirstChildElement(XML_ELM_CONFIG);
  if (configElement == nullptr)
  {
    CLog::Log(LOGERROR, "Missing element <{}> in element <{}>", XML_ELM_CONFIG,
              controllerElement.Name());
    return false;
  }

  // Get config appearance
  const char* configAppearance = configElement->Attribute(XML_ATTR_CONFIG_APPEARANCE);
  if (configAppearance == nullptr)
  {
    CLog::Log(LOGERROR, "Missing attribute \"{}\" in element <{}>", XML_ATTR_CONFIG_APPEARANCE,
              configElement->Name());
    return false;
  }

  controller.SetControllerAppearance(configAppearance);

  // Get config left deadzone
  const char* configLeftDeadzone = configElement->Attribute(XML_ATTR_CONFIG_LEFT_DEADZONE);
  if (configLeftDeadzone != nullptr)
    controller.SetLeftDeadzone(std::atof(configLeftDeadzone));

  // Get config right deadzone
  const char* configRightDeadzone = configElement->Attribute(XML_ATTR_CONFIG_RIGHT_DEADZONE);
  if (configRightDeadzone != nullptr)
    controller.SetRightDeadzone(std::atof(configRightDeadzone));

  return true;
}

bool CAgentControllerXML::DeserializeLastActive(const tinyxml2::XMLElement& controllerElement,
                                                CAgentController& controller)
{
  // Get last active element
  const tinyxml2::XMLElement* lastActiveElement =
      controllerElement.FirstChildElement(XML_ELM_LAST_ACTIVE);
  if (lastActiveElement != nullptr)
  {
    // Get last active text
    const char* lastActiveText = lastActiveElement->GetText();

    // Set last active UTC
    CDateTime lastActiveUtc;
    if (!lastActiveUtc.SetFromW3CDateTime(lastActiveText, true))
    {
      CLog::Log(LOGERROR, "Invalid text in element <{}>: \"{}\"", XML_ELM_LAST_ACTIVE,
                lastActiveText);
      return false;
    }

    controller.SetLastActiveUTC(lastActiveUtc);
  }

  return true;
}
