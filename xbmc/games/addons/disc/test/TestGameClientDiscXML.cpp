/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto GAME_PATH = "/roms/my_game.m3u";

void CleanupStateFile()
{
  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(GAME_PATH));
}

std::string ReadStateXml()
{
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  if (!file.Open(xmlPath))
    return "";

  std::string xml;
  xml.resize(static_cast<size_t>(file.GetLength()));
  if (!xml.empty())
    file.Read(xml.data(), xml.size());

  file.Close();
  return xml;
}
} // namespace

TEST(TestGameClientDiscXML, SaveLoadRoundtripPreservesSlotTypes)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd", "Disc One"));
  ASSERT_TRUE(savedModel.AddRemovedSlot());
  ASSERT_TRUE(savedModel.AddRemovedSlot());

  ASSERT_TRUE(savedModel.SetSelectedDiscByIndex(0));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 3U);
  EXPECT_TRUE(loadedModel.IsRealDiscByIndex(0));
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(2));
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetLabelByIndex(0), "Disc One");

  ASSERT_TRUE(loadedModel.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*loadedModel.GetSelectedDiscIndex(), 0U);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveLoadSelectedNonePreserved)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetSelectedNoDisc();

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_TRUE(loadedModel.IsSelectedNoDisc());
  EXPECT_FALSE(loadedModel.GetSelectedDiscIndex().has_value());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, MissingXmlIsNonErrorAndLeavesEmptyModel)
{
  CleanupStateFile();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscXML, MalformedXmlFailsAndClearsModel)
{
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char malformed[] = "<discstate><slots><slot type=\"disc\"></slots>";
  ASSERT_EQ(file.Write(malformed, sizeof(malformed) - 1), sizeof(malformed) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(loadedModel.AddDisc("/roms/placeholder.chd"));

  EXPECT_FALSE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, MergeAfterLoadPreservesRemovedTombstoneAgainstCoreRemoved)
{
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(loadedModel.AddRemovedSlot());
  ASSERT_TRUE(loadedModel.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel coreModel;
  ASSERT_TRUE(coreModel.AddRemovedSlot());
  ASSERT_TRUE(coreModel.AddDisc("/roms/disc2.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(loadedModel.GetDiscs(), coreModel.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
}

TEST(TestGameClientDiscXML, SaveWritesEjectedTrue)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"true\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveWritesEjectedFalse)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"false\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedState)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedFalseState)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadMissingEjectedDefaultsToFalse)
{
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char xml[] =
      "<discstate><slots><slot type=\"disc\" path=\"/roms/disc1.chd\"/></slots></discstate>";
  ASSERT_EQ(file.Write(xml, sizeof(xml) - 1), sizeof(xml) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}
