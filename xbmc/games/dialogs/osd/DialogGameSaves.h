/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DialogGameVideoSelect.h"
#include "FileItem.h"

namespace KODI
{
namespace GAME
{
class CDialogGameSaves : public CDialogGameVideoSelect
{
public:
  CDialogGameSaves();
  ~CDialogGameSaves() override = default;

protected:
  // implementation of CDialogGameVideoSelect
  std::string GetHeading() override;
  void PreInit() override;
  void GetItems(CFileItemList& items) override;
  void OnItemFocus(unsigned int index) override;
  unsigned int GetFocusedItem() const override;
  void PostExit() override;

private:
  void InitSavedGames();

  static void GetProperties(const CFileItem& item,
                            std::string& videoFilter,
                            std::string& description);

  CFileItemList m_items;
};
} // namespace GAME
} // namespace KODI
