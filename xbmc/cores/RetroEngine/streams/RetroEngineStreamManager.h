/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"

#include <map>
#include <memory>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
class IStreamManager;
}

namespace RETRO_ENGINE
{

class IRetroEngineStream;

class CRetroEngineStreamManager
{
public:
  CRetroEngineStreamManager();
  ~CRetroEngineStreamManager();

  // Rendering interface
  void Initialize(RETRO::IStreamManager& streamManager);
  void Deinitialize();

  std::unique_ptr<IRetroEngineStream> OpenStream(AVPixelFormat nominalFormat,
                                                 unsigned int nominalWidth,
                                                 unsigned int nominalHeight);
  void CloseStream(std::unique_ptr<IRetroEngineStream> stream);

private:
  // Utility functions
  std::unique_ptr<IRetroEngineStream> CreateStream() const;

  // Initialization parameters
  RETRO::IStreamManager* m_streamManager = nullptr;

  // Stream parameters
  std::map<IRetroEngineStream*, RETRO::StreamPtr> m_streams;
};

} // namespace RETRO_ENGINE
} // namespace KODI
