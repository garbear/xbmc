/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineStreamManager.h"

#include "cores/RetroEngine/streams/RetroEngineStreamSwFramebuffer.h"
#include "cores/RetroPlayer/streams/IRetroPlayerStream.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineStreamManager::CRetroEngineStreamManager()
{
}

CRetroEngineStreamManager::~CRetroEngineStreamManager() = default;

void CRetroEngineStreamManager::Initialize(RETRO::IStreamManager& streamManager)
{
  m_streamManager = &streamManager;
}

void CRetroEngineStreamManager::Deinitialize()
{
  m_streamManager = nullptr;
}

std::unique_ptr<IRetroEngineStream> CRetroEngineStreamManager::OpenStream(
    AVPixelFormat nominalFormat, unsigned int nominalWidth, unsigned int nominalHeight)
{
  if (m_streamManager == nullptr)
    return std::unique_ptr<IRetroEngineStream>{};

  std::unique_ptr<IRetroEngineStream> retroEngineStream = CreateStream();
  if (!retroEngineStream)
  {
    CLog::Log(LOGERROR, "RETROENGINE: Failed to create stream");
    return std::unique_ptr<IRetroEngineStream>{};
  }

  RETRO::StreamPtr retroStream = m_streamManager->CreateStream(RETRO::StreamType::SW_BUFFER);
  if (!retroStream)
  {
    CLog::Log(LOGERROR,
              "RETROENGINE: Invalid RetroPlayer stream type: RETRO::StreamType::SW_BUFFER");
    return std::unique_ptr<IRetroEngineStream>{};
  }

  // Force 0RGB32 format
  if (!retroEngineStream->OpenStream(retroStream.get(), AV_PIX_FMT_0RGB32, nominalWidth,
                                     nominalHeight))
  {
    CLog::Log(LOGERROR, "RETROENGINE: Failed to open stream");
    return std::unique_ptr<IRetroEngineStream>{};
  }

  m_streams[retroEngineStream.get()] = std::move(retroStream);

  return retroEngineStream;
}

void CRetroEngineStreamManager::CloseStream(std::unique_ptr<IRetroEngineStream> stream)
{
  if (stream)
  {
    stream->CloseStream();

    m_streamManager->CloseStream(std::move(m_streams[stream.get()]));
    m_streams.erase(stream.get());
  }
}

std::unique_ptr<IRetroEngineStream> CRetroEngineStreamManager::CreateStream() const
{
  std::unique_ptr<IRetroEngineStream> retroEngineStream;

  retroEngineStream.reset(new CRetroEngineStreamSwFramebuffer);

  return retroEngineStream;
}
