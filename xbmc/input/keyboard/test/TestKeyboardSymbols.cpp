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
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/input/PhysicalFeature.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace KEYBOARD;

TEST(TestKeyboardSymnbols, TranslateKeysym)
{
  // Load controller profile
  const CPhysicalFeature::ControllerPtr controller = CServiceBroker::GetGameServices().GetController(DEFAULT_KEYBOARD_ID);
  EXPECT_NE(controller.get(), nullptr);
  EXPECT_EQ(controller->ID(), DEFAULT_KEYBOARD_ID);

  //
  // Spec: Should translate all keyboard symbols
  //
  unsigned int count = 0;

  for (const GAME::CPhysicalFeature& feature : controller->Features())
  {
    const KEYBOARD::KeySymbol keycode = feature.Keycode();
    
    const char* symbolName = GAME::CControllerTranslator::TranslateKeycode(keycode);
    const KEYBOARD::KeySymbol keysym = GAME::CControllerTranslator::TranslateKeysym(symbolName);
    
    EXPECT_EQ(keycode, keysym);
    
    ++count;
  }
  
  EXPECT_EQ(count, 140);
}
