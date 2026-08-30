/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderCompileTypes.h"

#include <span>
#include <string_view>

namespace KODI::SHADER
{
struct ShaderPass;

class IShaderCompiler
{
public:
  virtual ~IShaderCompiler() = default;
  virtual std::string_view GetBackendId() const = 0;
  virtual ShaderCompileRequest CreateRequest(const ShaderPass& pass,
                                             ShaderCompileContext context) const = 0;
  virtual ShaderPrepareResult Prepare(const IShaderCompileInput& input) const = 0;
  virtual ShaderCompileResult Compile(const IShaderPreparedUnit& prepared) const = 0;
};

class IShaderArtifactStore
{
public:
  virtual ~IShaderArtifactStore() = default;
  virtual ShaderCacheLoadResult Load(const ShaderCompileKey& key) = 0;
  virtual bool Store(const ShaderCompileKey& key, std::span<const std::uint8_t> payload) = 0;
  virtual void Remove(const ShaderCompileKey& key) = 0;
};
} // namespace KODI::SHADER
