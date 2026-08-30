/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IShaderCompiler.h"

#include <cstdint>
#include <span>
#include <string>

namespace KODI::SHADER
{
class CShaderArtifactStore final : public IShaderArtifactStore
{
public:
  static constexpr std::uint64_t DEFAULT_MAX_PAYLOAD_SIZE{64ULL * 1024ULL * 1024ULL};

  CShaderArtifactStore(std::string root,
                       std::string extension,
                       std::uint32_t formatVersion = 1,
                       std::uint64_t maxPayloadSize = DEFAULT_MAX_PAYLOAD_SIZE);

  ShaderCacheLoadResult Load(const ShaderCompileKey& key) override;
  bool Store(const ShaderCompileKey& key, std::span<const std::uint8_t> payload) override;
  void Remove(const ShaderCompileKey& key) override;

  ShaderCacheLoadResult Load(std::string_view key);
  bool Store(std::string_view key, std::span<const std::uint8_t> payload);
  void Remove(std::string_view key);

private:
  bool IsValidKey(std::string_view key) const;
  std::string GetPath(std::string_view key) const;

  std::string m_root;
  std::string m_extension;
  std::uint32_t m_formatVersion;
  std::uint64_t m_maxPayloadSize;
};
} // namespace KODI::SHADER
