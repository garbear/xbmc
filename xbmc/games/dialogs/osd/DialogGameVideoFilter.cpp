/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVideoFilter.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "URL.h"

#include <stdlib.h>
#include "ServiceBroker.h"
#include "games/GameServices.h"

using namespace KODI;
using namespace GAME;

#define PRESETS_ADDON_NAME "game.shader.presets"

namespace
{
  struct ScalingMethodProperties
  {
    int nameIndex;
    int categoryIndex;
    int descriptionIndex;
    RETRO::SCALINGMETHOD scalingMethod;
  };

  const std::vector<ScalingMethodProperties> scalingMethods =
  {
    { 16301, 16296, 16298, RETRO::SCALINGMETHOD::NEAREST },
    { 16302, 16297, 16299, RETRO::SCALINGMETHOD::LINEAR },
  };
}

CDialogGameVideoFilter::CDialogGameVideoFilter() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

std::string CDialogGameVideoFilter::GetHeading()
{
  return g_localizeStrings.Get(35225); // "Video filter"
}

void CDialogGameVideoFilter::PreInit()
{
  m_items.Clear();

  InitScalingMethods();
  InitVideoFilters();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }

  m_bHasDescription = false;
}

void CDialogGameVideoFilter::InitScalingMethods()
{
  if (m_gameVideoHandle)
  {
    for (const auto &scalingMethodProps : scalingMethods)
    {
      if (m_gameVideoHandle->SupportsScalingMethod(scalingMethodProps.scalingMethod))
      {
        RETRO::CRenderVideoSettings videoSettings;
        videoSettings.SetScalingMethod(scalingMethodProps.scalingMethod);

        CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(scalingMethodProps.nameIndex));
        item->SetLabel2(g_localizeStrings.Get(scalingMethodProps.categoryIndex));
        item->SetProperty("game.videofilter", CVariant{ videoSettings.GetVideoFilter() });
        item->SetProperty("game.videofilterdescription", CVariant{ g_localizeStrings.Get(scalingMethodProps.descriptionIndex) });
        m_items.Add(std::move(item));
      }
    }
  }
}

void CDialogGameVideoFilter::InitVideoFilters()
{
  std::vector<VideoFilterProperties> videoFilters;

  // TODO: Have the add-on give us the xml as a string (or parse it)
  static const std::string addonPath = std::string("special://xbmcbinaddons/") + PRESETS_ADDON_NAME;
  static const std::string xmlPath = addonPath + "/resources/ShaderPresetsDefault.xml";
  std::string basePath = URIUtils::GetBasePath(xmlPath);

  CXBMCTinyXML xml = CXBMCTinyXML(xmlPath);

  if (!xml.LoadFile())
  {
    CLog::Log(LOGERROR, "%s - Couldn't load shader presets from default .xml, %s", __FUNCTION__, CURL::GetRedacted(xmlPath).c_str());
    return;
  }

  auto root = xml.RootElement();
  TiXmlNode* child = nullptr;

  while ((child = root->IterateChildren(child)))
  {
    VideoFilterProperties videoFilter;

    if (child->FirstChild() == nullptr)
      continue;

    TiXmlNode* pathNode;
    if ((pathNode = child->FirstChild("path")))
      if ((pathNode = pathNode->FirstChild()))
        videoFilter.path = URIUtils::AddFileToFolder(basePath, pathNode->Value());
    TiXmlNode* nameIndexNode;
    if ((nameIndexNode = child->FirstChild("name")))
      if ((nameIndexNode = nameIndexNode->FirstChild()))
        videoFilter.nameIndex = atoi(nameIndexNode->Value());
    TiXmlNode* categoryIndexNode;
    if ((categoryIndexNode = child->FirstChild("category")))
      if ((categoryIndexNode = categoryIndexNode->FirstChild()))
        videoFilter.categoryIndex = atoi(categoryIndexNode->Value());
    TiXmlNode* descriptionNode;
    if ((descriptionNode = child->FirstChild("description")))
      if ((descriptionNode = descriptionNode->FirstChild()))
        videoFilter.descriptionIndex = atoi(descriptionNode->Value());

    videoFilters.emplace_back(videoFilter);
  }

  CLog::Log(LOGDEBUG, "Loaded %d shader presets from default .xml, %s", videoFilters.size(), CURL::GetRedacted(xmlPath).c_str());

  for (const auto &videoFilter : videoFilters)
  {
    bool canLoadPreset = CServiceBroker::GetGameServices().VideoShaders().CanLoadPreset(videoFilter.path);

    if (!canLoadPreset)
      continue;

    auto localizedName = GetLocalizedString(videoFilter.nameIndex);
    auto localizedCategory = GetLocalizedString(videoFilter.categoryIndex);
    auto localizedDescription = GetLocalizedString(videoFilter.descriptionIndex);

    CFileItemPtr item = std::make_shared<CFileItem>(localizedName);
    item->SetLabel2(localizedCategory);
    item->SetProperty("game.videofilter", CVariant{ videoFilter.path });
    item->SetProperty("game.videofilterdescription", CVariant{ localizedDescription });

    m_items.Add(std::move(item));
  }
}

void CDialogGameVideoFilter::GetItems(CFileItemList &items)
{
  for (const auto &item : m_items)
    items.Add(item);
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
  {
    CFileItemPtr item = m_items[index];

    std::string videoFilter;
    std::string description;
    GetProperties(*item, videoFilter, description);

    ::CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

    if (gameSettings.VideoFilter() != videoFilter)
    {
      gameSettings.SetVideoFilter(videoFilter);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);

      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
    else if (!m_bHasDescription)
    {
      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  ::CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (int i = 0; i < m_items.Size(); i++)
  {
    std::string videoFilter;
    std::string description;
    GetProperties(*m_items[i], videoFilter, description);

    if (videoFilter == gameSettings.VideoFilter())
    {
      return i;
    }
  }

  return 0;
}

void CDialogGameVideoFilter::PostExit()
{
  m_items.Clear();
}

std::string CDialogGameVideoFilter::GetLocalizedString(uint32_t code)
{
  return g_localizeStrings.GetAddonString(PRESETS_ADDON_NAME, code);
}

void CDialogGameVideoFilter::GetProperties(const CFileItem &item, std::string &videoFilter, std::string &description)
{
  videoFilter = item.GetProperty("game.videofilter").asString();
  description = item.GetProperty("game.videofilterdescription").asString();
}
