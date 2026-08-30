/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "Util.h"
#include "cores/RetroPlayer/shaders/ShaderArtifactStore.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <array>
#include <cstdint>
#include <future>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::SHADER;

namespace
{
constexpr std::array<std::uint8_t, 8> MAGIC{'K', 'R', 'P', 'F', 'X', 'C', 0, 0};
constexpr std::uint64_t MAX_PAYLOAD_SIZE{64ULL * 1024ULL * 1024ULL};
const std::string KEY_A(64, 'a');
const std::vector<std::uint8_t> BYTES_A{0x01, 0x02, 0x03};

void AppendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
  for (unsigned int shift = 0; shift < 32; shift += 8)
    bytes.emplace_back(static_cast<std::uint8_t>(value >> shift));
}

void AppendU64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
{
  for (unsigned int shift = 0; shift < 64; shift += 8)
    bytes.emplace_back(static_cast<std::uint8_t>(value >> shift));
}

std::vector<std::uint8_t> RawKey(std::string_view key)
{
  std::vector<std::uint8_t> raw;
  raw.reserve(32);
  for (std::size_t i = 0; i < key.size(); i += 2)
    raw.emplace_back(
        static_cast<std::uint8_t>(std::stoul(std::string{key.substr(i, 2)}, nullptr, 16)));
  return raw;
}

void WriteAll(const std::string& path, const std::vector<std::uint8_t>& bytes)
{
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(path, true));
  ASSERT_EQ(static_cast<ssize_t>(bytes.size()), file.Write(bytes.data(), bytes.size()));
  file.Flush();
  file.Close();
}

void WriteEnvelope(const std::string& root,
                   std::string_view fileKey,
                   std::string_view embeddedKey,
                   std::span<const std::uint8_t> payload,
                   std::uint64_t declaredSize,
                   bool correctDigest)
{
  std::vector<std::uint8_t> bytes(MAGIC.begin(), MAGIC.end());
  AppendU32(bytes, 1);
  const auto rawKey = RawKey(embeddedKey);
  bytes.insert(bytes.end(), rawKey.begin(), rawKey.end());
  AppendU64(bytes, declaredSize);

  KODI::UTILITY::CDigest digest{KODI::UTILITY::CDigest::Type::SHA256};
  digest.Update(payload.data(), payload.size());
  const std::string rawDigest = digest.FinalizeRaw();
  bytes.insert(bytes.end(), rawDigest.begin(), rawDigest.end());
  if (!correctDigest)
    bytes[52] ^= 0xff;
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  WriteAll(URIUtils::AddFileToFolder(root, std::string{fileKey} + ".fxc"), bytes);
}

class ArtifactStoreRoot
{
public:
  ArtifactStoreRoot()
    : path(URIUtils::AddFileToFolder("special://temp/retroplayer-shader-tests/",
                                     StringUtils::CreateUUID()))
  {
    EXPECT_TRUE(CUtil::CreateDirectoryEx(path));
  }

  ~ArtifactStoreRoot() { XFILE::CDirectory::RemoveRecursive(path); }

  std::string path;
};
} // namespace

TEST(TestShaderArtifactStore, RoundTripValidEntry)
{
  // A stored payload must survive a complete envelope round trip unchanged.
  ArtifactStoreRoot root;
  CShaderArtifactStore store(root.path, ".fxc", 1, MAX_PAYLOAD_SIZE);
  ASSERT_TRUE(store.Store(KEY_A, BYTES_A));
  const ShaderCacheLoadResult result = store.Load(KEY_A);
  ASSERT_EQ(ShaderCacheLoadState::HIT, result.state);
  ASSERT_NE(nullptr, result.bytecode);
  EXPECT_EQ(BYTES_A, *result.bytecode);
}

TEST(TestShaderArtifactStore, RejectsTruncatedOversizedWrongKeyAndWrongDigest)
{
  // Every envelope field is validated before the payload is exposed or allocated.
  ArtifactStoreRoot root;
  CShaderArtifactStore store(root.path, ".fxc", 1, MAX_PAYLOAD_SIZE);

  const std::vector<std::uint8_t> truncated(MAGIC.begin(), MAGIC.end());
  WriteAll(URIUtils::AddFileToFolder(root.path, KEY_A + ".fxc"), truncated);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);

  WriteEnvelope(root.path, KEY_A, KEY_A, BYTES_A, MAX_PAYLOAD_SIZE + 1, true);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);

  WriteEnvelope(root.path, KEY_A, std::string(64, 'b'), BYTES_A, BYTES_A.size(), true);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);

  WriteEnvelope(root.path, KEY_A, KEY_A, BYTES_A, BYTES_A.size(), false);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);
}

TEST(TestShaderArtifactStore, RejectsNonLowercaseOrNonSha256Key)
{
  // Invalid keys never reach path construction and cannot escape the cache root.
  ArtifactStoreRoot root;
  CShaderArtifactStore store(root.path, ".fxc", 1, MAX_PAYLOAD_SIZE);
  EXPECT_EQ(ShaderCacheLoadState::MISS, store.Load("../bad").state);
  EXPECT_FALSE(store.Store("ABC", BYTES_A));
}

TEST(TestShaderArtifactStore, ConcurrentPublicationLeavesOneValidEntryAndNoTemporaryFiles)
{
  // Racing writers publish the same immutable winner and remove every temporary sibling.
  ArtifactStoreRoot root;
  CShaderArtifactStore store1(root.path, ".fxc", 1, MAX_PAYLOAD_SIZE);
  CShaderArtifactStore store2(root.path, ".fxc", 1, MAX_PAYLOAD_SIZE);

  auto first = std::async(std::launch::async, [&] { return store1.Store(KEY_A, BYTES_A); });
  auto second = std::async(std::launch::async, [&] { return store2.Store(KEY_A, BYTES_A); });
  EXPECT_TRUE(first.get());
  EXPECT_TRUE(second.get());
  EXPECT_EQ(ShaderCacheLoadState::HIT, store1.Load(KEY_A).state);

  CFileItemList items;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(root.path, items, XFILE::CDirectory::CHints{}));
  ASSERT_EQ(1, items.Size());
  EXPECT_EQ(KEY_A + ".fxc", items[0]->GetLabel());
}
