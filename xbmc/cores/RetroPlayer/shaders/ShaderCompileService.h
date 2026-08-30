/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IShaderCompiler.h"
#include "ShaderTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

class CJobQueue;

namespace KODI::SHADER
{
namespace INTERNAL
{
struct RequestState;
struct ServiceState;
struct Dispatcher;
struct GroupState;
} // namespace INTERNAL

class CShaderCompileGroup;

class CShaderCompileHandle
{
public:
  ShaderCompileState GetState() const;
  std::uint64_t GetGeneration() const;
  ShaderRequestDisposition GetDisposition() const;
  std::shared_ptr<const ShaderCompiledArtifact> GetArtifact() const;
  std::string GetError() const;
  void AddCompletionCallback(std::function<void()> callback) const;

private:
  explicit CShaderCompileHandle(std::shared_ptr<INTERNAL::RequestState> state);
  std::shared_ptr<INTERNAL::RequestState> m_state;
  friend class CShaderCompileService;
};

class CShaderCompileService
{
public:
  CShaderCompileService();
  ~CShaderCompileService();
  CShaderCompileService(const CShaderCompileService&) = delete;
  CShaderCompileService& operator=(const CShaderCompileService&) = delete;

  void RegisterCompiler(std::shared_ptr<IShaderCompiler> compiler,
                        std::shared_ptr<IShaderArtifactStore> store = {});
  bool SupportsAsyncCompilation(std::string_view backendId) const;
  std::shared_ptr<CShaderCompileHandle> Request(std::string_view backendId,
                                                const ShaderPass& pass,
                                                ShaderCompileContext context);
  std::shared_ptr<CShaderCompileGroup> RequestGroup(std::string_view backendId,
                                                    const std::vector<ShaderPass>& passes,
                                                    std::string presetPath,
                                                    std::function<void()> completion);
  bool RejectDiskArtifact(const std::shared_ptr<CShaderCompileHandle>& handle,
                          std::uint64_t generation);

private:
  std::shared_ptr<INTERNAL::ServiceState> m_state;
  std::shared_ptr<INTERNAL::Dispatcher> m_dispatcher;
  std::unique_ptr<CJobQueue> m_queue;
};

class CShaderCompileGroup
{
public:
  ShaderPresetState GetState() const;
  std::vector<std::shared_ptr<CShaderCompileHandle>> GetHandles() const;
  void AddCompletionCallback(std::function<void()> callback) const;

private:
  explicit CShaderCompileGroup(std::shared_ptr<INTERNAL::GroupState> state);
  std::shared_ptr<INTERNAL::GroupState> m_state;
  friend class CShaderCompileService;
};
} // namespace KODI::SHADER
