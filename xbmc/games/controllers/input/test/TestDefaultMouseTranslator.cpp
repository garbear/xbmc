/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/input/DefaultMouseTranslator.h"
#include "games/controllers/input/PhysicalFeature.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
// Helper functions
JOYSTICK::FEATURE_TYPE GetButtonType(MOUSE::BUTTON_ID buttonId, const ControllerPtr& controller, unsigned int& count)
{
  std::string buttonName = CDefaultMouseTranslator::TranslateMouseButton(buttonId);
  ++count;
  return controller->GetFeature(buttonName).Type();
}

JOYSTICK::FEATURE_TYPE GetPointerType(MOUSE::POINTER_DIRECTION direction, const ControllerPtr& controller)
{
  std::string pointerName = CDefaultMouseTranslator::TranslateMousePointer(direction);
  return controller->GetFeature(pointerName).Type();
}
}

TEST(TestDefaultKeyboardTranslator, TranslateKeycode)
{
  // Load controller profile
  const ControllerPtr controller = CServiceBroker::GetGameServices().GetController(DEFAULT_MOUSE_ID);
  EXPECT_NE(controller.get(), nullptr);
  EXPECT_EQ(controller->ID(), DEFAULT_MOUSE_ID);

  //
  // Spec: Should translate all mouse buttons
  //
  unsigned int count = 0;

  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::LEFT, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::RIGHT, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::MIDDLE, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::BUTTON4, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::BUTTON5, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::WHEEL_UP, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::WHEEL_DOWN, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::HORIZ_WHEEL_LEFT, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);
  EXPECT_EQ(GetButtonType(MOUSE::BUTTON_ID::HORIZ_WHEEL_RIGHT, controller, count), JOYSTICK::FEATURE_TYPE::SCALAR);

  EXPECT_EQ(count, 9);

  //
  // Spec: Should translate mouse pointer
  //
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::UP, controller), JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::DOWN, controller), JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::RIGHT, controller), JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::LEFT, controller), JOYSTICK::FEATURE_TYPE::RELPOINTER);
  EXPECT_EQ(GetPointerType(MOUSE::POINTER_DIRECTION::NONE, controller), JOYSTICK::FEATURE_TYPE::UNKNOWN);
}
