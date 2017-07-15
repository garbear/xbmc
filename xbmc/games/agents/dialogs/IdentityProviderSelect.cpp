/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IdentityProviderSelect.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

CIdentityProviderSelect::CIdentityProviderSelect() : CThread("IdentityProviderSelect")
{
}

CIdentityProviderSelect::~CIdentityProviderSelect() = default;

void CIdentityProviderSelect::Initialize()
{
  // Create thread
  Create(false);
}

void CIdentityProviderSelect::Deinitialize()
{
  // Stop thread
  StopThread(true);
}

void CIdentityProviderSelect::Process()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  CGUIWindowManager& windowManager = gui->GetWindowManager();

  auto pSelectDialog = windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (pSelectDialog == nullptr)
    return;

  CFileItemList items;

  {
    CFileItemPtr item(new CFileItem("Local keystore"));
    item->SetLabel2("Securely store identity in a local keystore.");
    item->SetArt("icon", "special://xbmc/addons/identity.did.key/resources/icon.png");
    items.Add(std::move(item));
  }

  {
    CFileItemPtr item(new CFileItem("GitHub Decentralized ID"));
    item->SetLabel2("Securely store identity in a GitHub repo.");
    item->SetArt("icon", "special://xbmc/addons/identity.did.github/resources/icon.png");
    items.Add(std::move(item));
  }

  {
    CFileItemPtr item(new CFileItem("InterPlanetary File System"));
    item->SetLabel2("Securely store identity on IPFS.");
    item->SetArt("icon", "special://xbmc/addons/identity.did.ipfs/resources/icon.png");
    items.Add(std::move(item));
  }

  pSelectDialog->Reset();
  pSelectDialog->SetHeading(CVariant{"Select an identity provider"});
  pSelectDialog->SetUseDetails(true);
  pSelectDialog->EnableButton(false, "");
  pSelectDialog->SetButtonFocus(false);
  for (const auto& it : items)
    pSelectDialog->Add(*it);
  pSelectDialog->Open();

  // If the thread was stopped, exit early
  if (m_bStop)
    return;

  if (pSelectDialog->IsConfirmed())
  {
    /*
    IdentityProviderPtr resultIdentityProvider;

    const int selected = pSelectDialog->GetSelectedItem();
    if (0 <= selected && selected < static_cast<int>(m_controllers.size()))
      resultIdentityProvider = m_controllers.at(selected);

    // Fire a callback with the result
    m_callback(resultIdentityProvider);
    */
  }
}
