/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class CGUIBaseContainer;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameDiscManager : public CGUIDialog
{
public:
  CDialogGameDiscManager();
  ~CDialogGameDiscManager() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;

  // Disc menu interface
  void SelectDiscToInsert(std::optional<size_t> selectedIndex,
                          std::function<void(std::optional<size_t>)> callback);
  void SelectDiscToRemove(std::function<void(size_t)> callback);
  void OnDiscSelect(size_t discIndex, bool isNoDisc);
  bool AllowSelectNoDisc() const;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Helper functions
  void InitializeDialog();
  void ResetDiscList();
  unsigned int GetSelectedIndex(std::optional<size_t> selectedIndex);
  void ShowControl(int controlId);

  CGUIBaseContainer* GetMenu();
  CGUIBaseContainer* GetDiscList();

  // Game parameters
  GameClientPtr m_gameClient;

  // Dialog parameters
  std::function<void(std::optional<size_t>)> m_insertCallback;
  std::function<void(size_t)> m_removeCallback;
};
} // namespace GAME
} // namespace KODI
