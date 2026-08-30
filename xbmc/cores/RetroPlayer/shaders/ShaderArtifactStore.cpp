/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderArtifactStore.h"

#include "Util.h"
#include "filesystem/File.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <vector>

using namespace KODI::UTILITY;

namespace KODI::SHADER
{
namespace
{
constexpr std::array<std::uint8_t, 8> MAGIC{'K', 'R', 'P', 'F', 'X', 'C', 0, 0};
constexpr std::size_t KEY_SIZE{32};
constexpr std::size_t DIGEST_SIZE{32};
constexpr std::size_t HEADER_SIZE{MAGIC.size() + sizeof(std::uint32_t) + KEY_SIZE +
                                  sizeof(std::uint64_t) + DIGEST_SIZE};

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

std::uint32_t ReadU32(const std::uint8_t* bytes)
{
  std::uint32_t value{0};
  for (unsigned int shift = 0; shift < 32; shift += 8)
    value |= static_cast<std::uint32_t>(*bytes++) << shift;
  return value;
}

std::uint64_t ReadU64(const std::uint8_t* bytes)
{
  std::uint64_t value{0};
  for (unsigned int shift = 0; shift < 64; shift += 8)
    value |= static_cast<std::uint64_t>(*bytes++) << shift;
  return value;
}

std::array<std::uint8_t, KEY_SIZE> DecodeKey(std::string_view key)
{
  std::array<std::uint8_t, KEY_SIZE> raw{};
  for (std::size_t i = 0; i < raw.size(); ++i)
  {
    const auto nibble = [](char value)
    {
      return value <= '9' ? static_cast<unsigned int>(value - '0')
                          : static_cast<unsigned int>(value - 'a' + 10);
    };
    raw[i] = static_cast<std::uint8_t>((nibble(key[i * 2]) << 4) | nibble(key[i * 2 + 1]));
  }
  return raw;
}

std::array<std::uint8_t, DIGEST_SIZE> PayloadDigest(std::span<const std::uint8_t> payload)
{
  CDigest digest{CDigest::Type::SHA256};
  digest.Update(payload.data(), payload.size());
  const std::string raw = digest.FinalizeRaw();
  std::array<std::uint8_t, DIGEST_SIZE> result{};
  std::copy(raw.begin(), raw.end(), result.begin());
  return result;
}

bool ReadAll(XFILE::CFile& file, std::span<std::uint8_t> destination)
{
  while (!destination.empty())
  {
    const ssize_t bytesRead = file.Read(destination.data(), destination.size());
    if (bytesRead <= 0)
      return false;
    destination = destination.subspan(static_cast<std::size_t>(bytesRead));
  }
  return true;
}

bool WriteAll(XFILE::CFile& file, std::span<const std::uint8_t> source)
{
  while (!source.empty())
  {
    const ssize_t bytesWritten = file.Write(source.data(), source.size());
    if (bytesWritten <= 0)
      return false;
    source = source.subspan(static_cast<std::size_t>(bytesWritten));
  }
  return true;
}
} // namespace

CShaderArtifactStore::CShaderArtifactStore(std::string root,
                                           std::string extension,
                                           std::uint32_t formatVersion,
                                           std::uint64_t maxPayloadSize)
  : m_root(std::move(root)),
    m_extension(std::move(extension)),
    m_formatVersion(formatVersion),
    m_maxPayloadSize(maxPayloadSize)
{
}

bool CShaderArtifactStore::IsValidKey(std::string_view key) const
{
  return key.size() == KEY_SIZE * 2 &&
         std::all_of(key.begin(), key.end(), [](unsigned char value)
                     { return std::isdigit(value) != 0 || (value >= 'a' && value <= 'f'); });
}

std::string CShaderArtifactStore::GetPath(std::string_view key) const
{
  return URIUtils::AddFileToFolder(m_root, std::string{key} + m_extension);
}

ShaderCacheLoadResult CShaderArtifactStore::Load(const ShaderCompileKey& key)
{
  return Load(key.hex);
}

ShaderCacheLoadResult CShaderArtifactStore::Load(std::string_view key)
{
  if (!IsValidKey(key))
    return {};

  const std::string path = GetPath(key);
  XFILE::CFile file;
  if (!file.Open(path))
    return {};

  const std::int64_t fileSize = file.GetLength();
  const auto corrupt = [&]()
  {
    file.Close();
    XFILE::CFile::Delete(path);
    return ShaderCacheLoadResult{ShaderCacheLoadState::CORRUPT, {}};
  };

  if (fileSize < static_cast<std::int64_t>(HEADER_SIZE) ||
      fileSize > static_cast<std::int64_t>(HEADER_SIZE + m_maxPayloadSize))
    return corrupt();

  std::array<std::uint8_t, HEADER_SIZE> header{};
  if (!ReadAll(file, header))
    return corrupt();

  const std::uint8_t* cursor = header.data();
  if (!std::equal(MAGIC.begin(), MAGIC.end(), cursor))
    return corrupt();
  cursor += MAGIC.size();

  if (ReadU32(cursor) != m_formatVersion)
    return corrupt();
  cursor += sizeof(std::uint32_t);

  const auto rawKey = DecodeKey(key);
  if (!std::equal(rawKey.begin(), rawKey.end(), cursor))
    return corrupt();
  cursor += KEY_SIZE;

  const std::uint64_t payloadSize = ReadU64(cursor);
  cursor += sizeof(std::uint64_t);
  if (payloadSize > m_maxPayloadSize ||
      payloadSize != static_cast<std::uint64_t>(fileSize - HEADER_SIZE))
    return corrupt();

  std::array<std::uint8_t, DIGEST_SIZE> expectedDigest{};
  std::copy_n(cursor, DIGEST_SIZE, expectedDigest.begin());

  auto payload = std::make_shared<std::vector<std::uint8_t>>(static_cast<std::size_t>(payloadSize));
  if (!ReadAll(file, *payload))
    return corrupt();
  file.Close();

  if (PayloadDigest(*payload) != expectedDigest)
  {
    XFILE::CFile::Delete(path);
    return {ShaderCacheLoadState::CORRUPT, {}};
  }

  return {ShaderCacheLoadState::HIT, std::move(payload)};
}

bool CShaderArtifactStore::Store(const ShaderCompileKey& key, std::span<const std::uint8_t> payload)
{
  return Store(key.hex, payload);
}

bool CShaderArtifactStore::Store(std::string_view key, std::span<const std::uint8_t> payload)
{
  if (!IsValidKey(key) || payload.size() > m_maxPayloadSize)
    return false;

  const std::string finalPath = GetPath(key);
  if (Load(key).state == ShaderCacheLoadState::HIT)
    return true;

  if (!CUtil::CreateDirectoryEx(m_root))
    return false;

  std::vector<std::uint8_t> envelope;
  envelope.reserve(HEADER_SIZE + payload.size());
  envelope.insert(envelope.end(), MAGIC.begin(), MAGIC.end());
  AppendU32(envelope, m_formatVersion);
  const auto rawKey = DecodeKey(key);
  envelope.insert(envelope.end(), rawKey.begin(), rawKey.end());
  AppendU64(envelope, payload.size());
  const auto digest = PayloadDigest(payload);
  envelope.insert(envelope.end(), digest.begin(), digest.end());
  envelope.insert(envelope.end(), payload.begin(), payload.end());

  const std::string temporaryPath = URIUtils::AddFileToFolder(
      m_root, std::string{key} + "." + StringUtils::CreateUUID() + ".tmp");
  XFILE::CFile file;
  if (!file.OpenForWrite(temporaryPath, false))
    return false;

  const bool written = WriteAll(file, envelope);
  if (written)
    file.Flush();
  file.Close();
  if (!written)
  {
    XFILE::CFile::Delete(temporaryPath);
    return false;
  }

  if (XFILE::CFile::Rename(temporaryPath, finalPath))
    return true;

  const bool winnerIsValid = Load(key).state == ShaderCacheLoadState::HIT;
  XFILE::CFile::Delete(temporaryPath);
  return winnerIsValid;
}

void CShaderArtifactStore::Remove(const ShaderCompileKey& key)
{
  Remove(key.hex);
}

void CShaderArtifactStore::Remove(std::string_view key)
{
  if (IsValidKey(key))
    XFILE::CFile::Delete(GetPath(key));
}
} // namespace KODI::SHADER
