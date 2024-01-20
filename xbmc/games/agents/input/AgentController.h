/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "XBDateTime.h"
#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace PERIPHERALS
{
class CPeripheral;
class CPeripheralJoystick;
} // namespace PERIPHERALS

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace KODI
{
namespace GAME
{
class CAgentJoystick;
class CAgentKeyboard;
class CAgentMouse;

/*!
 * \ingroup games
 *
 * \brief Class to represent the controller of a game player (a.k.a. agent)
 *
 * The term "agent" is used to distinguish game players from the myriad of other
 * uses of the term "player" in Kodi, such as media players and player cores.
 */
class CAgentController
{
public:
  using USB_ID = std::array<uint8_t, 2>;

  CAgentController();
  CAgentController(PERIPHERALS::PeripheralPtr peripheral);
  ~CAgentController();

  void ClearProperties();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Input accessors
  PERIPHERALS::PeripheralPtr GetPeripheral() const { return m_peripheral; }
  std::string GetControllerName() const;
  const std::string& GetPeripheralLocation() const;
  ControllerPtr GetController() const;
  CDateTime LastActiveUTC() const;
  float GetActivation() const;
  bool IsConnected() const;

  // Input mutators
  void ClearButtonState();

  // Peripheral accessors
  unsigned int GetControllerID() const { return m_controllerId; }
  PERIPHERALS::PeripheralType GetPeripheralType() const { return m_peripheralType; }
  PERIPHERALS::PeripheralBusType GetPeripheralBus() const { return m_peripheralBus; }
  std::string GetPeripheralVIDHex() const;
  std::string GetPeripheralPIDHex() const;
  std::string GetPeripheralLastLocation() const { return m_peripheralLastLocation; }
  int GetPeripheralOrdinal() const { return m_peripheralOrdinal; }

  // Peripheral mutators
  void SetControllerID(unsigned int controllerId) { m_controllerId = controllerId; }
  void SetPeripheralType(PERIPHERALS::PeripheralType peripheralType)
  {
    m_peripheralType = peripheralType;
  }
  void SetPeripheralBus(PERIPHERALS::PeripheralBusType peripheralBus)
  {
    m_peripheralBus = peripheralBus;
  }
  void SetPeripheralVIDHex(const std::string& vid);
  void SetPeripheralPIDHex(const std::string& pid);
  void SetPeripheralLastLocation(std::string peripheralLastLocation)
  {
    m_peripheralLastLocation = std::move(peripheralLastLocation);
  }
  void SetPeripheralOrdinal(unsigned int peripheralOrdinal)
  {
    m_peripheralOrdinal = peripheralOrdinal;
  }
  void AttachPeripheral(PERIPHERALS::PeripheralPtr peripheral);
  void DetachPeripheral();

  // Joystick accessors
  std::string GetJoystickName() const { return m_joystickName; }
  std::string GetJoystickProvider() const { return m_joystickProvider; }
  unsigned int GetJoystickButtons() const { return m_joystickButtons; }
  unsigned int GetJoystickHats() const { return m_joystickHats; }
  unsigned int GetJoystickAxes() const { return m_joystickAxes; }
  int GetJoystickRequestedPort() const { return m_joystickRequestedPort; }

  // Joystick mutators
  void SetJoystickName(std::string joystickName) { m_joystickName = std::move(joystickName); }
  void SetJoystickProvider(std::string joystickProvider)
  {
    m_joystickProvider = std::move(joystickProvider);
  }
  void SetJoystickButtons(unsigned int joystickButtons) { m_joystickButtons = joystickButtons; }
  void SetJoystickHats(unsigned int joystickHats) { m_joystickHats = joystickHats; }
  void SetJoystickAxes(unsigned int joystickAxes) { m_joystickAxes = joystickAxes; }
  void SetJoystickRequestedPort(int joystickRequestedPort)
  {
    m_joystickRequestedPort = joystickRequestedPort;
  }

  // Controller configuration accessors
  std::string GetControllerAppearance() const { return m_controllerAppearance; }
  float GetLeftDeadzone() const { return m_leftDeadzone; }
  float GetRightDeadzone() const { return m_rightDeadzone; }

  // Controller configuration mutators
  void SetControllerAppearance(const std::string& controllerAppearance)
  {
    m_controllerAppearance = controllerAppearance;
  }
  void SetLeftDeadzone(float leftDeadzone) { m_leftDeadzone = leftDeadzone; }
  void SetRightDeadzone(float rightDeadzone) { m_rightDeadzone = rightDeadzone; }

  // Controller state accessors
  CDateTime GetLastActiveUTC() const { return m_lastActiveUtc; }

  // Controller state mutators
  void SetLastActiveUTC(CDateTime m_lastActiveUtc) { m_lastActiveUtc = m_lastActiveUtc; }

  // XML functions
  bool Serialize(tinyxml2::XMLElement& controllerElement) const;
  bool Deserialize(const tinyxml2::XMLElement& controllerElement);

private:
  // Initialization helper functions
  void InitControllerParameters(const ControllerPtr& controller);
  void InitPeripheralParameters(const PERIPHERALS::CPeripheral& peripheral);
  void InitJoystickParameters(const PERIPHERALS::CPeripheralJoystick& peripheralJoystick);

  // Helper functions
  static USB_ID ParseUsbId(const std::string& strUsbId);

  // Construction parameters
  PERIPHERALS::PeripheralPtr m_peripheral;

  // Input parameters
  std::unique_ptr<CAgentJoystick> m_joystick;
  std::unique_ptr<CAgentKeyboard> m_keyboard;
  std::unique_ptr<CAgentMouse> m_mouse;

  // Controller parameters
  unsigned int m_controllerId{0};

  // Peripheral parameters
  PERIPHERALS::PeripheralType m_peripheralType;
  PERIPHERALS::PeripheralBusType m_peripheralBus;
  USB_ID m_peripheralVid;
  USB_ID m_peripheralPid;
  std::string m_peripheralLastLocation;
  unsigned int m_peripheralOrdinal;

  // Joystick parameters
  std::string m_joystickName;
  std::string m_joystickProvider;
  unsigned int m_joystickButtons;
  unsigned int m_joystickHats;
  unsigned int m_joystickAxes;
  int m_joystickRequestedPort;

  // Controller configuration
  std::string m_controllerAppearance;
  float m_leftDeadzone;
  float m_rightDeadzone;

  // Controller state
  CDateTime m_lastActiveUtc;
};

} // namespace GAME
} // namespace KODI
