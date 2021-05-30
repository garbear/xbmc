/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeStreams.h"

#include "cores/RetroPlayer/streams/IRetroPlayerStream.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "smarthome/SmartHomeServices.h"
#include "smarthome/streams/SmartHomeStreamSwFramebuffer.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeStreams::CSmartHomeStreams(CSmartHomeServices& smartHome) : m_smartHome(smartHome)
{
}

CSmartHomeStreams::~CSmartHomeStreams() = default;

void CSmartHomeStreams::Initialize(RETRO::IStreamManager& streamManager)
{
  m_streamManager = &streamManager;
}

void CSmartHomeStreams::Deinitialize()
{
  m_streamManager = nullptr;
}

ISmartHomeStream* CSmartHomeStreams::OpenStream(AVPixelFormat nominalFormat,
                                                unsigned int nominalWidth,
                                                unsigned int nominalHeight)
{
  if (m_streamManager == nullptr)
    return nullptr;

  std::unique_ptr<ISmartHomeStream> smartHomeStream = CreateStream();
  if (!smartHomeStream)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to create stream");
    return nullptr;
  }

  RETRO::StreamPtr retroStream = m_streamManager->CreateStream(RETRO::StreamType::SW_BUFFER);
  if (!retroStream)
  {
    CLog::Log(LOGERROR, "SMARTHOME: Invalid RetroPlayer stream type: RETRO::StreamType::SW_BUFFER");
    return nullptr;
  }

  // Force 0RGB32 format
  if (!smartHomeStream->OpenStream(retroStream.get(), AV_PIX_FMT_0RGB32, nominalWidth,
                                   nominalHeight))
  {
    CLog::Log(LOGERROR, "SMARTHOME: Failed to open stream");
    return nullptr;
  }

  m_streams[smartHomeStream.get()] = std::move(retroStream);

  return smartHomeStream.release();
}

void CSmartHomeStreams::CloseStream(ISmartHomeStream* stream)
{
  if (stream != nullptr)
  {
    std::unique_ptr<ISmartHomeStream> streamHolder(stream);
    streamHolder->CloseStream();

    m_streamManager->CloseStream(std::move(m_streams[stream]));
    m_streams.erase(stream);
  }
}

std::unique_ptr<ISmartHomeStream> CSmartHomeStreams::CreateStream() const
{
  std::unique_ptr<ISmartHomeStream> smartHomeStream;

  smartHomeStream.reset(new CSmartHomeStreamSwFramebuffer);

  return smartHomeStream;
}
