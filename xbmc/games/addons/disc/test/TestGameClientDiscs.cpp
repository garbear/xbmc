/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
struct DiscCore
{
  std::vector<std::string> slots;
  unsigned int selected{0};
  bool ejected{false};
  unsigned int mutations{0};
  bool failReplace{false};
  bool compactRemoval{false};
  std::string initialPath;
  unsigned int initialIndex{0};
  bool failInitialImage{false};
  unsigned int deserializeCalls{0};
  std::vector<std::string> deserializedSlots;
  std::string machinePath;
};

DiscCore& Core(const AddonInstance_Game* instance)
{
  return *static_cast<DiscCore*>(instance->toAddon->addonInstance);
}
} // namespace

namespace KODI::GAME
{
class TestGameClientDiscs : public testing::Test
{
protected:
  void SetUp() override
  {
    CXBMCTinyXML2 xml;
    const std::string addonXml = R"(<addon id="game.test.discs" name="Disc test" version="1.0.0">
      <extension point="kodi.gameclient" library="test.so">
        <supports_disc_control>true</supports_disc_control>
        <extensions>chd|m3u</extensions>
      </extension>
      <extension point="kodi.addon.metadata"><platform>all</platform></extension>
    </addon>)";
    ASSERT_TRUE(xml.Parse(addonXml));
    const auto info =
        ADDON::CAddonInfoBuilder::Generate(xml.RootElement(), ADDON::RepositoryDirInfo{});
    ASSERT_NE(info, nullptr);
    m_client = std::make_unique<CGameClient>(info);
    auto* callbacks = m_client->GetInstanceInterface()->toAddon;
    callbacks->addonInstance = &m_core;
    callbacks->GetEjectState = [](const AddonInstance_Game* game) { return Core(game).ejected; };
    callbacks->SetEjectState = [](const AddonInstance_Game* game, bool ejected)
    {
      Core(game).ejected = ejected;
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImageCount = [](const AddonInstance_Game* game)
    { return static_cast<unsigned int>(Core(game).slots.size()); };
    callbacks->GetImageIndex = [](const AddonInstance_Game* game) { return Core(game).selected; };
    callbacks->SetImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      Core(game).selected = index;
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->AddImageIndex = [](const AddonInstance_Game* game)
    {
      Core(game).slots.emplace_back();
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->ReplaceImageIndex =
        [](const AddonInstance_Game* game, unsigned int index, const char* path)
    {
      auto& core = Core(game);
      if (core.failReplace || index >= core.slots.size() || !core.ejected)
        return GAME_ERROR_FAILED;
      core.slots[index] = path ? path : "";
      ++core.mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->RemoveImageIndex = [](const AddonInstance_Game* game, unsigned int index)
    {
      if (index >= Core(game).slots.size())
        return GAME_ERROR_FAILED;
      if (Core(game).compactRemoval)
        Core(game).slots.erase(Core(game).slots.begin() + index);
      else
        Core(game).slots[index].clear();
      ++Core(game).mutations;
      return GAME_ERROR_NO_ERROR;
    };
    callbacks->SetInitialImage =
        [](const AddonInstance_Game* game, unsigned int index, const char* path)
    {
      Core(game).initialIndex = index;
      Core(game).initialPath = path ? path : "";
      return Core(game).failInitialImage ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR;
    };
    callbacks->GetImagePath = [](const AddonInstance_Game* game, unsigned int index) -> char*
    {
      return index < Core(game).slots.size() && !Core(game).slots[index].empty()
                 ? strdup(Core(game).slots[index].c_str())
                 : nullptr;
    };
    callbacks->GetImageLabel = [](const AddonInstance_Game*, unsigned int) -> char*
    { return nullptr; };
    callbacks->FreeString = [](const AddonInstance_Game*, char* string) { free(string); };
    callbacks->Deserialize = [](const AddonInstance_Game* game, const uint8_t* data, size_t size)
    {
      auto& core = Core(game);
      ++core.deserializeCalls;
      core.deserializedSlots = core.slots;
      if (size < 2 || core.selected != data[0] || core.selected >= core.slots.size())
        return GAME_ERROR_FAILED;
      const std::string path(reinterpret_cast<const char*>(data + 1), size - 1);
      if (core.slots[core.selected] != path)
        return GAME_ERROR_FAILED;
      core.machinePath = path;
      return GAME_ERROR_NO_ERROR;
    };
  }

  void TearDown() override
  {
    if (m_client)
      m_client->m_bIsPlaying = false;
    for (auto* file : m_files)
    {
      const std::string path = XBMC_TEMPFILEPATH(file);
      XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(path));
      XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(path));
      EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
    }
  }

  std::string CreateFile(const std::string& extension, const std::string& contents = {})
  {
    auto* file = XBMC_CREATETEMPFILE(extension);
    EXPECT_NE(file, nullptr);
    if (!file)
      return {};
    m_files.push_back(file);
    if (!contents.empty())
      EXPECT_EQ(file->Write(contents.data(), contents.size()),
                static_cast<ssize_t>(contents.size()));
    file->Close();
    return XBMC_TEMPFILEPATH(file);
  }

  void StartPlaying()
  {
    m_client->m_bIsPlaying = true;
    m_client->m_hasFrameRun = true;
    m_client->Discs().RefreshDiscState();
  }

  bool Deserialize(const CGameClientDiscModel& target, uint8_t selected, const std::string& path)
  {
    std::vector<uint8_t> data{selected};
    data.insert(data.end(), path.begin(), path.end());
    return m_client->Deserialize(data.data(), data.size(), &target);
  }

  DiscCore m_core;
  std::unique_ptr<CGameClient> m_client;
  std::vector<XFILE::CFile*> m_files;
};
} // namespace KODI::GAME

TEST_F(TestGameClientDiscs, OlderDiscModelKeepsLaterMediaIdentity)
{
  const std::string path = CreateFile(".chd");
  ASSERT_FALSE(path.empty());
  CGameClientDiscModel later;
  later.AddDisc(path);
  const auto state = later.GetState();
  m_client->Discs().SetDiscModel(later);
  m_client->Discs().SetDiscModel({});

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), path);
}

TEST_F(TestGameClientDiscs, UnchangedDiscModelCanLearnMediaIdentity)
{
  const std::string path = CreateFile(".chd");
  ASSERT_FALSE(path.empty());
  CGameClientDiscModel history;
  history.AddDisc(path);
  const auto state = history.GetState();
  ASSERT_TRUE(history.EraseDiscByIndex(0));
  m_client->Discs().SetDiscModel(history);

  CGameClientDiscModel restored;
  ASSERT_TRUE(m_client->Discs().GetDiscs().ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), path);
}

TEST_F(TestGameClientDiscs, EmptyPersistedStateLearnsOriginalPlaylistWithoutActivatingIt)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, {}));
  CGameClientDiscModel historical;
  historical.AddDisc(disc);

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  EXPECT_TRUE(current.IsSelectedNoDisc());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(historical.GetState(), restored));
  EXPECT_EQ(restored.GetPathByIndex(0), disc);
}

TEST_F(TestGameClientDiscs, StartupPruningPreservesRemovedMediaFromXML)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(disc);
  const auto state = previous.GetState();
  ASSERT_TRUE(previous.MarkRemovedByIndex(0));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, previous));

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_TRUE(current.Empty());
  EXPECT_TRUE(current.IsSelectedNoDisc());
  CGameClientDiscModel restored;
  ASSERT_TRUE(current.ResolveState(state, restored));
  EXPECT_EQ(restored.GetPathByIndex(0), disc);
}

TEST_F(TestGameClientDiscs, MissingHistoricalMediaDoesNotInvalidateCurrentPlaylist)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel persisted;
  persisted.AddDisc(disc);
  persisted.RememberDiscPath("/missing/historical-disc.chd");
  persisted.SetEjected(true);
  ASSERT_TRUE(CGameClientDiscXML::Save(playlist, persisted));

  m_client->Discs().Initialize(playlist);

  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current, persisted);
  CGameClientDiscModel missing;
  missing.AddDisc("/missing/historical-disc.chd");
  CGameClientDiscModel resolved;
  EXPECT_FALSE(current.ResolveState(missing.GetState(), resolved));
}

TEST_F(TestGameClientDiscs, NewSessionDiscardsPreviousMediaCatalog)
{
  const std::string previousDisc = CreateFile(".chd");
  const std::string nextDisc = CreateFile(".chd");
  ASSERT_FALSE(previousDisc.empty());
  ASSERT_FALSE(nextDisc.empty());
  m_client->Discs().Initialize(previousDisc);
  const auto previousState = m_client->Discs().GetDiscs().GetState();
  m_client->Discs().Deinitialize();
  EXPECT_TRUE(m_client->Discs().GetDiscs().GetKnownDiscPaths().empty());

  m_client->Discs().Initialize(nextDisc);

  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.GetKnownDiscPaths(), std::vector<std::string>{nextDisc});
  CGameClientDiscModel resolved;
  EXPECT_FALSE(current.ResolveState(previousState, resolved));
}

TEST_F(TestGameClientDiscs, InvalidPersistedSelectionFallsBackToSource)
{
  const std::string disc = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", disc + "\n");
  ASSERT_FALSE(disc.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(disc);
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("selected")->SetAttribute("index", 5);
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.GetPathByIndex(0), disc);
  EXPECT_EQ(current.GetSelectedDiscIndex(), 0U);
}

TEST_F(TestGameClientDiscs, MalformedPersistedSlotCannotShiftSelectionOntoAnotherDisc)
{
  const std::string first = CreateFile(".chd");
  const std::string second = CreateFile(".chd");
  const std::string playlist = CreateFile(".m3u", first + "\n" + second + "\n");
  ASSERT_FALSE(first.empty());
  ASSERT_FALSE(second.empty());
  ASSERT_FALSE(playlist.empty());
  CGameClientDiscModel previous;
  previous.AddDisc(first);
  previous.AddDisc(second);
  ASSERT_TRUE(CGameClientDiscXML::Save(playlist, previous));
  CXBMCTinyXML2 document;
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(playlist);
  ASSERT_TRUE(document.LoadFile(xmlPath));
  document.RootElement()->FirstChildElement("slots")->FirstChildElement("slot")->DeleteAttribute(
      "path");
  ASSERT_TRUE(document.SaveFile(xmlPath));

  m_client->Discs().Initialize(playlist);

  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  const auto current = m_client->Discs().GetDiscs();
  EXPECT_EQ(current.Size(), 2U);
  EXPECT_EQ(current.GetSelectedDiscPath(), first);
}

TEST_F(TestGameClientDiscs, DeserializePreparesMediaAfterEmptyPlaylist)
{
  m_core.ejected = true;
  StartPlaying();
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc1.chd");
  target.AddDisc("/roms/disc2.chd");
  ASSERT_TRUE(target.SetSelectedDiscByIndex(1));

  EXPECT_TRUE(Deserialize(target, 1, "/roms/disc2.chd"));

  EXPECT_EQ(m_core.machinePath, "/roms/disc2.chd");
  EXPECT_EQ(m_core.selected, 1U);
  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestGameClientDiscs, DeserializePreparesReorderedMedia)
{
  m_core.slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
  StartPlaying();
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc1.chd");
  target.AddDisc("/roms/disc2.chd");
  ASSERT_TRUE(target.SetSelectedDiscByIndex(1));

  EXPECT_TRUE(Deserialize(target, 1, "/roms/disc2.chd"));

  EXPECT_EQ(m_core.machinePath, "/roms/disc2.chd");
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 1U);
}

TEST_F(TestGameClientDiscs, FailedMediaPreparationSkipsDeserializeAndRestoresPreviousState)
{
  m_core.slots = {"/roms/disc1.chd"};
  StartPlaying();
  const CGameClientDiscModel previous = m_client->Discs().GetDiscs();
  m_core.failReplace = true;
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc2.chd");
  target.SetEjected(true);

  EXPECT_FALSE(Deserialize(target, 0, "/roms/disc2.chd"));

  EXPECT_EQ(m_core.deserializeCalls, 0U);
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
}

TEST_F(TestGameClientDiscs, FailedDeserializeRestoresPreviousMediaAndTray)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  const CGameClientDiscModel previous = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    ++Core(game).deserializeCalls;
    Core(game).deserializedSlots = Core(game).slots;
    return GAME_ERROR_FAILED;
  };
  CGameClientDiscModel target;
  target.AddDisc("/roms/disc2.chd");

  EXPECT_FALSE(Deserialize(target, 0, "/roms/disc2.chd"));

  EXPECT_EQ(m_core.deserializedSlots, (std::vector<std::string>{"/roms/disc2.chd"}));
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == previous);
}

TEST_F(TestGameClientDiscs, DeserializeReconcilesMediaChangedByCore)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  CGameClientDiscModel target = m_client->Discs().GetDiscs();
  target.SetEjected(true);
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    Core(game).selected = 1;
    Core(game).ejected = false;
    return GAME_ERROR_NO_ERROR;
  };

  EXPECT_TRUE(Deserialize(target, 0, "/roms/disc1.chd"));

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, DeserializeReconcilesMediaWithoutPathMetadata)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  CGameClientDiscModel target = m_client->Discs().GetDiscs();
  target.SetEjected(true);
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    Core(game).selected = 1;
    Core(game).ejected = false;
    return GAME_ERROR_NO_ERROR;
  };

  EXPECT_TRUE(Deserialize(target, 0, "/roms/disc1.chd"));

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, FailedDeserializeRestoresUnchangedModelWithoutPathMetadata)
{
  m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
  StartPlaying();
  const CGameClientDiscModel target = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    Core(game).slots = {"/roms/disc2.chd", "/roms/disc1.chd"};
    return GAME_ERROR_FAILED;
  };

  EXPECT_FALSE(Deserialize(target, 0, "/roms/disc1.chd"));

  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "/roms/disc2.chd"}));
  EXPECT_EQ(m_core.selected, 0U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, DeserializeRetriesClosedTrayWithoutLosingSavedOpenTray)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  const CGameClientDiscModel target = m_client->Discs().GetDiscs();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  {
    ++Core(game).deserializeCalls;
    return Core(game).ejected ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR;
  };

  EXPECT_TRUE(Deserialize(target, 0, "/roms/disc1.chd"));

  EXPECT_EQ(m_core.deserializeCalls, 2U);
  EXPECT_TRUE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == target);
}

TEST_F(TestGameClientDiscs, LegacyDeserializeClosesTrayBeforeLoading)
{
  m_core.slots = {"/roms/disc1.chd"};
  m_core.ejected = true;
  StartPlaying();
  m_client->GetInstanceInterface()->toAddon->Deserialize =
      [](const AddonInstance_Game* game, const uint8_t*, size_t)
  { return Core(game).ejected ? GAME_ERROR_FAILED : GAME_ERROR_NO_ERROR; };
  const uint8_t data = 0;

  EXPECT_TRUE(m_client->Deserialize(&data, sizeof(data)));

  EXPECT_FALSE(m_core.ejected);
}

TEST_F(TestGameClientDiscs, RestoreHistoricalTopologyClearsRemovedAndExtraSlots)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd", "One");
  historical.AddRemovedSlot();
  historical.AddDisc("/roms/disc2.chd", "Two");
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(2));
  m_core.slots = {"/roms/disc2.chd", "/roms/reused.chd", "/roms/disc1.chd", "/roms/extra.chd"};
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.slots, (std::vector<std::string>{"/roms/disc1.chd", "", "/roms/disc2.chd", ""}));
  EXPECT_EQ(m_core.selected, 2U);
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  m_core.mutations = 0;
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.mutations, 0U);
  m_client->Discs().RefreshDiscState();
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
}

TEST_F(TestGameClientDiscs, SourceRetryReplacesPersistedInitialHint)
{
  std::unique_ptr<XFILE::CFile> firstDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> secondDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(firstDisc, nullptr);
  ASSERT_NE(secondDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  firstDisc->Close();
  secondDisc->Close();
  const std::string firstPath = XBMC_TEMPFILEPATH(firstDisc.get());
  const std::string secondPath = XBMC_TEMPFILEPATH(secondDisc.get());
  const std::string source = firstPath + "\n" + secondPath + "\n";
  ASSERT_EQ(playlist->Write(source.data(), source.size()), static_cast<ssize_t>(source.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(firstPath);
  persisted.AddDisc(secondPath);
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_client->Discs().Initialize(gamePath);
  EXPECT_EQ(m_core.initialIndex, 1U);
  EXPECT_EQ(m_core.initialPath, secondPath);

  m_client->Discs().Initialize(gamePath, false);
  EXPECT_EQ(m_core.initialIndex, 0U);
  EXPECT_EQ(m_core.initialPath, firstPath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(secondDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(firstDisc.release()));
}

TEST_F(TestGameClientDiscs, SourceRetryReplacesPersistedHintAfterCallbackFailure)
{
  std::unique_ptr<XFILE::CFile> firstDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> secondDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(firstDisc, nullptr);
  ASSERT_NE(secondDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  firstDisc->Close();
  secondDisc->Close();
  const std::string firstPath = XBMC_TEMPFILEPATH(firstDisc.get());
  const std::string secondPath = XBMC_TEMPFILEPATH(secondDisc.get());
  const std::string source = firstPath + "\n" + secondPath + "\n";
  ASSERT_EQ(playlist->Write(source.data(), source.size()), static_cast<ssize_t>(source.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(firstPath);
  persisted.AddDisc(secondPath);
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_core.failInitialImage = true;
  m_client->Discs().Initialize(gamePath);
  EXPECT_EQ(m_core.initialIndex, 1U);
  EXPECT_EQ(m_core.initialPath, secondPath);

  m_client->Discs().Initialize(gamePath, false);
  EXPECT_EQ(m_core.initialIndex, 0U);
  EXPECT_EQ(m_core.initialPath, firstPath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(secondDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(firstDisc.release()));
}

TEST_F(TestGameClientDiscs, RestoreSelectionAndTrayIndependently)
{
  for (const bool ejected : {false, true})
  {
    for (const bool noDisc : {false, true})
    {
      CGameClientDiscModel historical;
      historical.AddDisc("/roms/disc1.chd");
      historical.AddDisc("/roms/disc2.chd");
      ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));
      if (noDisc)
        historical.SetSelectedNoDisc();
      historical.SetEjected(ejected);
      m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
      m_core.selected = 0;
      m_core.ejected = !ejected;
      m_client->Discs().SetDiscModel(historical);
      ASSERT_TRUE(m_client->Discs().RestoreDiscList());
      EXPECT_EQ(m_core.selected, noDisc ? 2U : 1U);
      EXPECT_EQ(m_core.ejected, ejected);
    }
  }
}

TEST_F(TestGameClientDiscs, NoDiscAcceptsCoreSentinelBeyondImageCount)
{
  m_client->GetInstanceInterface()->toAddon->GetImageIndex = [](const AddonInstance_Game* game)
  {
    return Core(game).selected < Core(game).slots.size() ? Core(game).selected
                                                         : std::numeric_limits<unsigned int>::max();
  };
  for (const bool ejected : {false, true})
  {
    CGameClientDiscModel historical;
    historical.AddDisc("/roms/disc1.chd", "One");
    historical.AddDisc("/roms/disc2.chd", "Two");
    historical.SetSelectedNoDisc();
    historical.SetEjected(ejected);
    m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
    m_core.selected = 0;
    m_core.ejected = !ejected;
    m_client->Discs().SetDiscModel(historical);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_EQ(m_core.selected, 2U);
    EXPECT_EQ(m_core.ejected, ejected);
    m_core.mutations = 0;
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_EQ(m_core.mutations, 0U);
    m_client->Discs().RefreshDiscState();
    EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  }
}

TEST_F(TestGameClientDiscs, PrepareDeserializePreservesHistoricalTrayModel)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.SetEjected(true);
  m_client->Discs().SetDiscModel(historical);
  m_core.ejected = true;
  ASSERT_TRUE(m_client->Discs().PrepareForDeserialize());
  EXPECT_FALSE(m_core.ejected);
  EXPECT_TRUE(m_client->Discs().GetDiscs() == historical);
  m_core.mutations = 0;
  ASSERT_TRUE(m_client->Discs().PrepareForDeserialize());
  EXPECT_EQ(m_core.mutations, 0U);
}

TEST_F(TestGameClientDiscs, RestoreEmptyPlaylistAndTrailingRemovedSlot)
{
  m_core.slots = {"/roms/disc1.chd"};
  CGameClientDiscModel empty;
  m_client->Discs().SetDiscModel(empty);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_TRUE(m_core.slots[0].empty());
  EXPECT_EQ(m_core.selected, m_core.slots.size());

  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.AddRemovedSlot();
  historical.AddRemovedSlot();
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.slots.size(), 3U);
  EXPECT_TRUE(m_core.slots[2].empty());
}

TEST_F(TestGameClientDiscs, RefusedMediaRestoreReportsFailure)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc2.chd");
  m_core.slots = {"/roms/disc1.chd"};
  m_core.failReplace = true;
  m_client->Discs().SetDiscModel(historical);
  EXPECT_FALSE(m_client->Discs().RestoreDiscList());
}

TEST_F(TestGameClientDiscs, PresentEmptyPersistedStateIsAuthoritative)
{
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(playlist, nullptr);
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscXML xml;
  CGameClientDiscModel empty;
  ASSERT_TRUE(xml.Save(gamePath, empty));

  m_client->Discs().Initialize(gamePath);
  EXPECT_TRUE(m_client->Discs().HasPersistedState());
  EXPECT_TRUE(m_client->Discs().GetDiscs().Empty());

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
}

TEST_F(TestGameClientDiscs, InvalidPersistedStateFallsBackToWholeSourcePlaylist)
{
  std::unique_ptr<XFILE::CFile> disc(XBMC_CREATETEMPFILE(".chd"));
  ASSERT_NE(disc, nullptr);
  disc->Close();
  const std::string discPath = XBMC_TEMPFILEPATH(disc.get());

  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(playlist, nullptr);
  ASSERT_EQ(playlist->Write(discPath.data(), discPath.size()),
            static_cast<ssize_t>(discPath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel stale;
  stale.AddDisc("/missing/disc.chd");
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, stale));

  m_client->Discs().Initialize(gamePath);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), discPath);
  EXPECT_EQ(source.GetSelectedDiscIndex(), 0U);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(disc.release()));
}

TEST_F(TestGameClientDiscs, UnsupportedPersistedMediaFallsBackToWholeSourcePlaylist)
{
  std::unique_ptr<XFILE::CFile> sourceDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> unsupportedDisc(XBMC_CREATETEMPFILE(".iso"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(sourceDisc, nullptr);
  ASSERT_NE(unsupportedDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  sourceDisc->Close();
  unsupportedDisc->Close();
  const std::string sourcePath = XBMC_TEMPFILEPATH(sourceDisc.get());
  ASSERT_EQ(playlist->Write(sourcePath.data(), sourcePath.size()),
            static_cast<ssize_t>(sourcePath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel incompatible;
  incompatible.AddDisc(XBMC_TEMPFILEPATH(unsupportedDisc.get()));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, incompatible));

  m_client->Discs().Initialize(gamePath);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), sourcePath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(unsupportedDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(sourceDisc.release()));
}

TEST_F(TestGameClientDiscs, SourceOnlyInitializationIgnoresValidPersistedState)
{
  std::unique_ptr<XFILE::CFile> sourceDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> persistedDisc(XBMC_CREATETEMPFILE(".chd"));
  std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
  ASSERT_NE(sourceDisc, nullptr);
  ASSERT_NE(persistedDisc, nullptr);
  ASSERT_NE(playlist, nullptr);
  sourceDisc->Close();
  persistedDisc->Close();
  const std::string sourcePath = XBMC_TEMPFILEPATH(sourceDisc.get());
  ASSERT_EQ(playlist->Write(sourcePath.data(), sourcePath.size()),
            static_cast<ssize_t>(sourcePath.size()));
  playlist->Close();
  const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

  CGameClientDiscModel persisted;
  persisted.AddDisc(XBMC_TEMPFILEPATH(persistedDisc.get()));
  CGameClientDiscXML xml;
  ASSERT_TRUE(xml.Save(gamePath, persisted));

  m_client->Discs().Initialize(gamePath, false);
  const CGameClientDiscModel source = m_client->Discs().GetDiscs();
  EXPECT_FALSE(m_client->Discs().HasPersistedState());
  ASSERT_EQ(source.Size(), 1U);
  EXPECT_EQ(source.GetPathByIndex(0), sourcePath);

  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(persistedDisc.release()));
  EXPECT_TRUE(XBMC_DELETETEMPFILE(sourceDisc.release()));
}

TEST_F(TestGameClientDiscs, PersistedEmptyAndShortModelsRestoreAcrossCoreRemovalBehaviors)
{
  for (const bool compactRemoval : {false, true})
  {
    std::unique_ptr<XFILE::CFile> disc(XBMC_CREATETEMPFILE(".chd"));
    std::unique_ptr<XFILE::CFile> playlist(XBMC_CREATETEMPFILE(".m3u"));
    ASSERT_NE(disc, nullptr);
    ASSERT_NE(playlist, nullptr);
    disc->Close();
    playlist->Close();
    const std::string gamePath = XBMC_TEMPFILEPATH(playlist.get());

    for (const bool empty : {false, true})
    {
      CGameClientDiscModel persisted;
      if (!empty)
        persisted.AddDisc(XBMC_TEMPFILEPATH(disc.get()));
      CGameClientDiscXML xml;
      ASSERT_TRUE(xml.Save(gamePath, persisted));

      m_core.compactRemoval = compactRemoval;
      m_core.slots = {"/core/one.chd", "/core/two.chd", "/core/three.chd"};
      m_core.selected = 0;
      m_client->Discs().Initialize(gamePath);
      ASSERT_TRUE(m_client->Discs().HasPersistedState());
      ASSERT_TRUE(m_client->Discs().RestoreDiscList());
      m_client->Discs().RefreshDiscState();

      const CGameClientDiscModel restored = m_client->Discs().GetDiscs();
      EXPECT_EQ(restored.Size(), empty ? 0U : 1U);
      EXPECT_EQ(m_core.slots.size(), compactRemoval ? (empty ? 0U : 1U) : 3U);
      if (!compactRemoval)
      {
        for (size_t i = empty ? 0U : 1U; i < m_core.slots.size(); ++i)
          EXPECT_TRUE(m_core.slots[i].empty());
      }
    }

    XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(gamePath));
    XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(gamePath));
    EXPECT_TRUE(XBMC_DELETETEMPFILE(playlist.release()));
    EXPECT_TRUE(XBMC_DELETETEMPFILE(disc.release()));
  }
}

TEST_F(TestGameClientDiscs, AddAfterHistoricalRestoreReusesClearedPhysicalTail)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.SetEjected(true);
  m_core.slots = {"/roms/disc1.chd", "/roms/future.chd"};
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc2.chd"));
  const auto model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc2.chd");
  EXPECT_EQ(m_core.slots.size(), 2U);
}

TEST_F(TestGameClientDiscs, ReusedRemovedSlotKeepsIdentityWithoutCoreMetadata)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd");
  historical.AddRemovedSlot();
  historical.SetEjected(true);
  m_core.slots = {"/roms/disc1.chd", ""};
  m_core.ejected = true;
  m_client->Discs().SetDiscModel(historical);
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  ASSERT_TRUE(m_client->Discs().AddDisc("/roms/disc2.chd"));
  const auto model = m_client->Discs().GetDiscs();
  EXPECT_EQ(model.Size(), 2U);
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_EQ(model.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST_F(TestGameClientDiscs, RepeatedRestoreWithoutCorePathMetadataDoesNotMutateMedia)
{
  CGameClientDiscModel historical;
  historical.AddDisc("/roms/disc1.chd", "One");
  historical.AddDisc("/roms/disc2.chd", "Two");
  ASSERT_TRUE(historical.SetSelectedDiscByIndex(1));
  m_core.slots = {"/roms/stale1.chd", "/roms/stale2.chd"};
  m_core.selected = 1;
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };

  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_GT(m_core.mutations, 0U);

  m_core.mutations = 0;
  m_client->Discs().SetDiscModel(historical);
  ASSERT_TRUE(m_client->Discs().RestoreDiscList());
  EXPECT_EQ(m_core.mutations, 0U);
}

TEST_F(TestGameClientDiscs, UnknownCorePathsCannotBypassHistoricalRemoval)
{
  m_client->GetInstanceInterface()->toAddon->GetImagePath =
      [](const AddonInstance_Game*, unsigned int) -> char* { return nullptr; };
  for (const bool removedSlot : {false, true})
  {
    CGameClientDiscModel historical;
    if (removedSlot)
      historical.AddRemovedSlot();
    m_core.slots = {"/roms/disc1.chd", "/roms/disc2.chd"};
    m_client->Discs().SetDiscModel(historical);
    ASSERT_TRUE(m_client->Discs().RestoreDiscList());
    EXPECT_TRUE(m_core.slots[0].empty());
    EXPECT_TRUE(m_core.slots[1].empty());
    EXPECT_EQ(m_core.selected, 2U);
    m_client->Discs().RefreshDiscState();
    EXPECT_EQ(m_client->Discs().GetDiscs().Size(), historical.Size());
  }
}
