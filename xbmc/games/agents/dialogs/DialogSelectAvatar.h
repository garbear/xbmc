/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

class CFileItemList;
class CGUIViewControl;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogSelectAvatar : public CGUIDialog
{
public:
  CDialogSelectAvatar();
  ~CDialogSelectAvatar() override = default;

  // implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

  // Player interface
  bool IsPlayerOneReady();

protected:
  // implementation of CGUIWIndow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;

private:
  /*!
   * \brief Called when an item has been selected
   */
  void OnSelect(unsigned int itemIndex);

  // Dialog parameters
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;

  // Dialog state
  int m_selectedItem{-1};
};
} // namespace GAME
} // namespace KODI
