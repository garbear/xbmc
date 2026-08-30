/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace KODI::SHADER
{
enum class ShaderCompileState
{
  UNKNOWN,
  QUEUED,
  COMPILING,
  READY,
  FAILED
};

enum class ShaderArtifactOrigin
{
  DISK,
  COMPILED
};

enum class ShaderRequestDisposition
{
  MEMORY_HIT,
  DISK_HIT,
  QUEUED,
  COALESCED
};

enum class ShaderCompileIdentityKind
{
  CANONICAL,
  PREPARATION_FAILURE
};

struct ShaderCompileIdentity
{
  ShaderCompileIdentityKind kind{ShaderCompileIdentityKind::CANONICAL};
  std::string backendId;
  std::string value;

  bool operator==(const ShaderCompileIdentity&) const = default;
  auto operator<=>(const ShaderCompileIdentity&) const = default;
};

enum class ShaderPresetState
{
  READY,
  PENDING,
  FAILED
};

struct ShaderCompileKey
{
  std::array<std::uint8_t, 32> raw{};
  std::string hex;
};

struct ShaderCompileContext
{
  std::string presetPath;
  unsigned int passIndex{0};
  std::string passAlias;
  std::string shaderPath;
};

struct ShaderCompiledArtifact
{
  ShaderCompileKey key;
  std::shared_ptr<const std::vector<std::uint8_t>> bytecode;
  ShaderArtifactOrigin origin{ShaderArtifactOrigin::COMPILED};
};

struct ShaderCompileTerminal
{
  ShaderCompileIdentity identity;
  std::uint64_t generation{0};
  ShaderCompileState state{ShaderCompileState::UNKNOWN};
  ShaderRequestDisposition disposition{ShaderRequestDisposition::QUEUED};
  std::optional<ShaderArtifactOrigin> artifactOrigin;
};

enum class ShaderCacheLoadState
{
  MISS,
  HIT,
  CORRUPT
};

struct ShaderCacheLoadResult
{
  ShaderCacheLoadState state{ShaderCacheLoadState::MISS};
  std::shared_ptr<const std::vector<std::uint8_t>> bytecode;
};

class IShaderCompileInput
{
public:
  virtual ~IShaderCompileInput() = default;
};

class IShaderPreparedUnit
{
public:
  virtual ~IShaderPreparedUnit() = default;
};

struct ShaderCompileRequest
{
  std::string provisionalKey;
  std::shared_ptr<const IShaderCompileInput> input;
  ShaderCompileContext context;
};

struct ShaderPrepareResult
{
  std::optional<ShaderCompileKey> canonicalKey;
  std::string failureFingerprint;
  std::shared_ptr<const IShaderPreparedUnit> prepared;
  std::string error;
};

struct ShaderCompileResult
{
  std::vector<std::uint8_t> bytecode;
  std::string error;
};
} // namespace KODI::SHADER
