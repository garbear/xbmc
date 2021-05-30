/*
 *  Copyright (C) 2021 Team Kodi
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

namespace SMART_HOME
{

class CSmartHomeServices;
class ISmartHomeStream;

class CSmartHomeStreams
{
public:
  CSmartHomeStreams(CSmartHomeServices& smartHome);
  ~CSmartHomeStreams();

  void Initialize(RETRO::IStreamManager& streamManager);
  void Deinitialize();

  ISmartHomeStream* OpenStream(AVPixelFormat nominalFormat,
                               unsigned int nominalWidth,
                               unsigned int nominalHeight);
  void CloseStream(ISmartHomeStream* stream);

private:
  // Utility functions
  std::unique_ptr<ISmartHomeStream> CreateStream() const;

  // Construction parameters
  CSmartHomeServices& m_smartHome;

  // Initialization parameters
  RETRO::IStreamManager* m_streamManager = nullptr;

  // Stream parameters
  std::map<ISmartHomeStream*, RETRO::StreamPtr> m_streams;
};

} // namespace SMART_HOME
} // namespace KODI
