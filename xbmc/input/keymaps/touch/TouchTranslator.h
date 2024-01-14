/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keymaps/interfaces/IKeyMapper.h"

#include <map>
#include <string>

class TiXmlElement;

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 */
class CTouchTranslator : public IKeyMapper
{
public:
  CTouchTranslator() = default;

  // implementation of IKeyMapper
  void MapActions(int windowID, const TiXmlNode* bDevice) override;
  void Clear() override;

  bool TranslateTouchAction(
      int window, int touchAction, int touchPointers, int& action, std::string& actionString);

private:
  bool TranslateAction(int window,
                       unsigned int touchCommand,
                       int touchPointers,
                       unsigned int& actionId,
                       std::string& actionString);

  struct CTouchAction
  {
    unsigned int actionId;
    std::string strAction; // Needed for "ActivateWindow()" type actions
  };

  using TouchActionKey = unsigned int;
  using TouchActionMap = std::map<TouchActionKey, CTouchAction>;

  using WindowID = int;
  using TouchMap = std::map<WindowID, TouchActionMap>;

  unsigned int GetActionID(WindowID window,
                           TouchActionKey touchActionKey,
                           std::string& actionString);

  static unsigned int TranslateTouchCommand(const TiXmlElement* pButton, CTouchAction& action);

  static unsigned int GetTouchActionKey(unsigned int touchCommandId, int touchPointers);

  TouchMap m_touchMap;
};
} // namespace KEYMAP
} // namespace KODI
