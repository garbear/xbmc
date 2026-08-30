/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderCompiler.h"
#include "guilib/D3DResource.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace KODI::SHADER
{
struct ShaderCompileInputDX final : IShaderCompileInput
{
  std::string sourcePath;
  std::string source;
  DefinesMap defines;
};

struct ShaderPreparedUnitDX final : IShaderPreparedUnit
{
  std::string preprocessedSource;
  std::vector<std::vector<std::uint8_t>> dependencies;
};

class CShaderCompilerDX final : public IShaderCompiler
{
public:
  static constexpr std::string_view BACKEND_ID{"d3d11-fx"};
  static constexpr std::string_view COMPILER_ID{"d3dcompiler_47"};
  static constexpr std::string_view EFFECTS11_VERSION{"1129"};
  static constexpr std::string_view TARGET{"fx_5_0"};
  static constexpr std::uint32_t CACHE_ABI{1};

  explicit CShaderCompilerDX(std::shared_ptr<std::atomic_uint> compileCounter = {});

  std::string_view GetBackendId() const override { return BACKEND_ID; }
  ShaderCompileRequest CreateRequest(const ShaderPass& pass,
                                     ShaderCompileContext context) const override;
  ShaderPrepareResult Prepare(const IShaderCompileInput& input) const override;
  ShaderCompileResult Compile(const IShaderPreparedUnit& prepared) const override;

private:
  std::shared_ptr<std::atomic_uint> m_compileCounter;
};
} // namespace KODI::SHADER
