/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "guilib/GUIButtonControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
class CGUIPortButton : public CGUIButtonControl
{
public:
  CGUIPortButton(const CGUIButtonControl& buttonControl,
                 const std::string& label,
                 ControllerPtr controller,
                 unsigned int index);

  ~CGUIPortButton() override = default;

  ControllerPtr GetController() const { return m_selectedController; }

private:
  static std::string GetLabel(const ControllerPtr& controller);

  const ControllerPtr m_selectedController;
};
} // namespace GAME
} // namespace KODI
