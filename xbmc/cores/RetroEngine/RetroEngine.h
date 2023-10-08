/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameClient;
} // namespace GAME

namespace RETRO_ENGINE
{

class CRetroEngineGuiBridge;
class CRetroEngineRenderer;
class CRetroEngineStreamManager;
class IRetroEngineStream;

class IRetroEngine
{
public:
  virtual ~IRetroEngine() = default;

  // Lifecycle functions
  virtual bool Initialize(std::vector<std::shared_ptr<GAME::CGameClient>> gameClients) = 0;
  virtual void Deinitialize() = 0;
};

class CRetroEngine : public IRetroEngine
{
public:
  explicit CRetroEngine(CRetroEngineGuiBridge& guiBridge, const std::string& savestatePath);
  ~CRetroEngine();

  // Implementation of IRetroEngine
  bool Initialize(std::vector<std::shared_ptr<GAME::CGameClient>> gameClients) override;
  void Deinitialize() override;

private:
  // Construction parameters
  CRetroEngineGuiBridge& m_guiBridge;
  const std::string m_savestatePath;

  // RetroEngine parameters
  std::unique_ptr<CRetroEngineStreamManager> m_streamManager;
  std::unique_ptr<IRetroEngineStream> m_stream;
  std::unique_ptr<CRetroEngineRenderer> m_renderer;
  std::vector<std::shared_ptr<GAME::CGameClient>> m_gameClients;
};
} // namespace RETRO_ENGINE
} // namespace KODI
