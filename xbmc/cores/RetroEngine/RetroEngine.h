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
namespace RETRO_ENGINE
{

class CRetroEngineGuiBridge;
class CRetroEngineRenderer;
class CRetroEngineStreamManager;
class IRetroEngineStream;

class CRetroEngine
{
public:
  explicit CRetroEngine(CRetroEngineGuiBridge& guiBridge, const std::string& savestatePath);
  ~CRetroEngine();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

private:
  // Construction parameters
  CRetroEngineGuiBridge& m_guiBridge;
  const std::string m_savestatePath;

  // RetroEngine parameters
  std::unique_ptr<CRetroEngineStreamManager> m_streamManager;
  std::unique_ptr<IRetroEngineStream> m_stream;
  std::unique_ptr<CRetroEngineRenderer> m_renderer;
};
} // namespace RETRO_ENGINE
} // namespace KODI
