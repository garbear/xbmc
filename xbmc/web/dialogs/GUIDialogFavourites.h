/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogBoxBase.h"
#include "view/GUIViewControl.h"
#include "web/Favourites.h"

#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

namespace WEB
{

class CWebDatabase;

class CGUIDialogWebFavourites : public CGUIDialogBoxBase
{
public:
  CGUIDialogWebFavourites();
  ~CGUIDialogWebFavourites(void) override;

  static std::string ShowAndGetURL();

protected:
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;
  CGUIControl *GetFirstFocusableControl(int id) override;
  void OnWindowLoaded() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void OnWindowUnload() override;

private:
  std::string ShowAndGetFavourite(CWebDatabase* database);
  void OnPopupMenu(int item);

  int m_selectedItem;
  std::unique_ptr<CFileItemList> m_vecList;
  CGUIViewControl m_viewControl;
  CWebDatabase* m_database;
};

}
