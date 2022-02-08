/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "datastore/DataStore.h"
#include "datastore/IDataStore.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace DATASTORE;

namespace
{
std::string TempDataStorePath(const std::string& name)
{
  const std::string path =
      URIUtils::AddFileToFolder("special://temp", "kodi_datastore_test_" + name);
  if (XFILE::CDirectory::Exists(path))
    XFILE::CDirectory::RemoveRecursive(path);
  return path;
}
} // namespace

TEST(TestDataStore, EmptyPathDoesNotOpen)
{
  CDataStore dataStore;

  EXPECT_FALSE(dataStore.Open(""));
}

TEST(TestDataStore, UnopenedStoreOperationsFailSafely)
{
  CDataStore dataStore;
  const uint8_t key[] = {1, 2};
  const uint8_t value[] = {3, 4};
  const uint8_t* data = nullptr;
  size_t dataSize = 99;

  EXPECT_FALSE(dataStore.Has(key, sizeof(key)));
  EXPECT_FALSE(dataStore.Get(key, sizeof(key), data, dataSize));
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(dataSize, 99U);
  EXPECT_EQ(dataStore.Reserve(key, sizeof(key), sizeof(value)), nullptr);
  EXPECT_FALSE(dataStore.Put(key, sizeof(key), value, sizeof(value)));
  EXPECT_FALSE(dataStore.Delete(key, sizeof(key)));

  dataStore.Release(nullptr);
  dataStore.Commit(nullptr);
  dataStore.Close();
}

TEST(TestDataStore, PutGetHasDeleteRoundTrip)
{
  const auto path = TempDataStorePath("round_trip");
  CDataStore dataStore;
  const uint8_t key[] = {1, 2, 3};
  const uint8_t value[] = {4, 5, 6, 7};

  ASSERT_TRUE(dataStore.Open(path));

  EXPECT_FALSE(dataStore.Has(key, sizeof(key)));
  ASSERT_TRUE(dataStore.Put(key, sizeof(key), value, sizeof(value)));
  EXPECT_TRUE(dataStore.Has(key, sizeof(key)));

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(key, sizeof(key), data, dataSize));
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(std::vector<uint8_t>(data, data + dataSize), (std::vector<uint8_t>{4, 5, 6, 7}));
  dataStore.Release(data);

  EXPECT_TRUE(dataStore.Delete(key, sizeof(key)));
  EXPECT_FALSE(dataStore.Has(key, sizeof(key)));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, OpenCreatesMissingDirectory)
{
  const auto path = TempDataStorePath("missing_directory");
  CDataStore dataStore;
  const uint8_t key[] = {1};
  const uint8_t value[] = {2};

  ASSERT_FALSE(XFILE::CDirectory::Exists(path));
  ASSERT_TRUE(dataStore.Open(path));
  EXPECT_TRUE(XFILE::CDirectory::Exists(path));
  ASSERT_TRUE(dataStore.Put(key, sizeof(key), value, sizeof(value)));

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(key, sizeof(key), data, dataSize));
  EXPECT_EQ(std::vector<uint8_t>(data, data + dataSize), (std::vector<uint8_t>{2}));
  dataStore.Release(data);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, OpenTranslatesSpecialTempPath)
{
  const std::string specialPath = "special://temp/Datastore/ipfs-test";
  const std::string translatedPath = CSpecialProtocol::TranslatePath(specialPath);
  CDataStore dataStore;
  const uint8_t key[] = {3};
  const uint8_t value[] = {4, 5};

  ASSERT_FALSE(translatedPath.empty());
  if (XFILE::CDirectory::Exists(translatedPath))
    XFILE::CDirectory::RemoveRecursive(translatedPath);

  ASSERT_TRUE(dataStore.Open(specialPath));
  EXPECT_TRUE(XFILE::CDirectory::Exists(translatedPath));
  ASSERT_TRUE(dataStore.Put(key, sizeof(key), value, sizeof(value)));

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(key, sizeof(key), data, dataSize));
  EXPECT_EQ(std::vector<uint8_t>(data, data + dataSize), (std::vector<uint8_t>{4, 5}));
  dataStore.Release(data);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(translatedPath);
}

TEST(TestDataStore, InvalidPointerSizePairsFailWithoutChangingOutputs)
{
  const auto path = TempDataStorePath("invalid_pointer_size_pairs");
  CDataStore dataStore;
  const uint8_t key[] = {1, 2, 3};
  const uint8_t value[] = {4, 5, 6};
  const uint8_t* data = value;
  size_t dataSize = 99;

  ASSERT_TRUE(dataStore.Open(path));

  EXPECT_FALSE(dataStore.Has(nullptr, 1));
  EXPECT_FALSE(dataStore.Get(nullptr, 1, data, dataSize));
  EXPECT_EQ(data, value);
  EXPECT_EQ(dataSize, 99U);
  EXPECT_EQ(dataStore.Reserve(nullptr, 1, sizeof(value)), nullptr);
  EXPECT_FALSE(dataStore.Put(nullptr, 1, value, sizeof(value)));
  EXPECT_FALSE(dataStore.Put(key, sizeof(key), nullptr, 1));
  EXPECT_FALSE(dataStore.Delete(nullptr, 1));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, PutReplacesExistingValue)
{
  const auto path = TempDataStorePath("replace");
  CDataStore dataStore;
  const uint8_t key[] = {1};
  const uint8_t first[] = {2};
  const uint8_t second[] = {3, 4};

  ASSERT_TRUE(dataStore.Open(path));
  ASSERT_TRUE(dataStore.Put(key, sizeof(key), first, sizeof(first)));
  ASSERT_TRUE(dataStore.Put(key, sizeof(key), second, sizeof(second)));

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(key, sizeof(key), data, dataSize));
  EXPECT_EQ(std::vector<uint8_t>(data, data + dataSize), (std::vector<uint8_t>{3, 4}));
  dataStore.Release(data);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, ReserveAndCommitStoresValue)
{
  const auto path = TempDataStorePath("reserve");
  CDataStore dataStore;
  const uint8_t key[] = {7, 8};

  ASSERT_TRUE(dataStore.Open(path));

  uint8_t* reserved = dataStore.Reserve(key, sizeof(key), 3);
  ASSERT_NE(reserved, nullptr);
  reserved[0] = 9;
  reserved[1] = 10;
  reserved[2] = 11;
  dataStore.Commit(reserved);

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(key, sizeof(key), data, dataSize));
  EXPECT_EQ(std::vector<uint8_t>(data, data + dataSize), (std::vector<uint8_t>{9, 10, 11}));
  dataStore.Release(data);

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, CloseAbortsOutstandingReadTransaction)
{
  const auto path = TempDataStorePath("close_outstanding_read");
  CDataStore dataStore;
  const uint8_t readKey[] = {1};
  const uint8_t readValue[] = {2, 3};

  ASSERT_TRUE(dataStore.Open(path));
  ASSERT_TRUE(dataStore.Put(readKey, sizeof(readKey), readValue, sizeof(readValue)));

  const uint8_t* data = nullptr;
  size_t dataSize = 0;
  ASSERT_TRUE(dataStore.Get(readKey, sizeof(readKey), data, dataSize));
  ASSERT_NE(data, nullptr);

  dataStore.Close();

  ASSERT_TRUE(dataStore.Open(path));
  EXPECT_TRUE(dataStore.Has(readKey, sizeof(readKey)));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}

TEST(TestDataStore, CloseAbortsOutstandingReservedWriteTransaction)
{
  const auto path = TempDataStorePath("close_outstanding_reserve");
  CDataStore dataStore;
  const uint8_t reserveKey[] = {4};

  ASSERT_TRUE(dataStore.Open(path));

  uint8_t* reserved = dataStore.Reserve(reserveKey, sizeof(reserveKey), 2);
  ASSERT_NE(reserved, nullptr);
  reserved[0] = 5;
  reserved[1] = 6;

  dataStore.Close();

  ASSERT_TRUE(dataStore.Open(path));
  EXPECT_FALSE(dataStore.Has(reserveKey, sizeof(reserveKey)));

  dataStore.Close();
  XFILE::CDirectory::RemoveRecursive(path);
}
