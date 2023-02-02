/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "peripherals/PeripheralTypes.h"

namespace KODI
{
namespace GAME
{
class CController;
class CControllerState;
} // namespace GAME

namespace RETRO_ENGINE
{
class IRetroEngineJoystickHandler;

/*!
 * \ingroup retroengine
 * \brief Handles game controller events for game engine functionality
 *
 * Listens to game controller events and forwards them to the game engine
 * messaging layer.
 */
class CRetroEngineJoystick : public JOYSTICK::IInputHandler
{
public:
  /*!
   * \brief Constructor
   * \param peripheral
   * \param controller The game controller which is used (for controller mapping).
   * \param joystickHandler
   */
  CRetroEngineJoystick(PERIPHERALS::PeripheralPtr peripheral,
                       GAME::ControllerPtr controller,
                       IRetroEngineJoystickHandler& joystickHandler);

  ~CRetroEngineJoystick() override;

  // Implementation of IJoystickHandler
  std::string ControllerID() const override;
  bool HasFeature(const std::string& feature) const override;
  bool AcceptsInput(const std::string& feature) const override;
  bool OnButtonPress(const std::string& feature, bool bPressed) override;
  void OnButtonHold(const std::string& feature, unsigned int holdTimeMs) override {}
  bool OnButtonMotion(const std::string& feature,
                      float magnitude,
                      unsigned int motionTimeMs) override;
  bool OnAnalogStickMotion(const std::string& feature,
                           float x,
                           float y,
                           unsigned int motionTimeMs) override;
  bool OnAccelerometerMotion(const std::string& feature, float x, float y, float z) override;
  bool OnWheelMotion(const std::string& feature,
                     float position,
                     unsigned int motionTimeMs) override;
  bool OnThrottleMotion(const std::string& feature,
                        float position,
                        unsigned int motionTimeMs) override;
  void OnInputFrame() override;

private:
  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;
  const GAME::ControllerPtr m_controller;
  IRetroEngineJoystickHandler& m_joystickHandler;

  // Input parameters
  std::unique_ptr<GAME::CControllerState> m_controllerState;
};
} // namespace RETRO_ENGINE
} // namespace KODI
