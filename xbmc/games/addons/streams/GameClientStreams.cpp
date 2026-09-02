/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreams.h"

#include "GameClientStreamAudio.h"
#include "ServiceBroker.h"
#include "rendering/RenderSystem.h"
#include "GameClientStreamHwFramebuffer.h"
#include "GameClientStreamSwFramebuffer.h"
#include "GameClientStreamVideo.h"
#include "cores/RetroPlayer/streams/IRetroPlayerStream.h"
#include "cores/RetroPlayer/streams/IStreamManager.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientTranslator.h"
#include "utils/log.h"

#include <memory>
#include <tuple>

using namespace KODI;
using namespace GAME;

CGameClientStreams::CGameClientStreams(CGameClient& gameClient) : m_gameClient(gameClient)
{
}

void CGameClientStreams::Initialize(RETRO::IStreamManager& streamManager)
{
  m_streamManager = &streamManager;
}

void CGameClientStreams::Deinitialize()
{
  m_streamManager = nullptr;
}

IGameClientStream* CGameClientStreams::OpenStream(const game_stream_properties& properties)
{
  if (m_streamManager == nullptr)
    return nullptr;

  RETRO::StreamType retroStreamType;
  if (!CGameClientTranslator::TranslateStreamType(properties.type, retroStreamType))
  {
    CLog::Log(LOGERROR, "GAME: Invalid stream type: {}", static_cast<int>(properties.type));
    return nullptr;
  }

  std::unique_ptr<IGameClientStream> gameStream = CreateStream(properties.type);
  if (!gameStream)
  {
    CLog::Log(LOGERROR, "GAME: No stream implementation for type: {}",
              static_cast<int>(properties.type));
    return nullptr;
  }

  RETRO::StreamPtr retroStream = m_streamManager->CreateStream(retroStreamType);
  if (!retroStream)
  {
    CLog::Log(LOGERROR, "GAME:  Invalid RetroPlayer stream type: {}",
              static_cast<int>(retroStreamType));
    return nullptr;
  }

  if (!gameStream->OpenStream(retroStream.get(), properties))
  {
    CLog::Log(LOGERROR, "GAME: Failed to open stream");
    return nullptr;
  }

  m_streams[gameStream.get()] = std::move(retroStream);

  return gameStream.release();
}

void CGameClientStreams::CloseStream(IGameClientStream* stream)
{
  if (stream != nullptr)
  {
    std::unique_ptr<IGameClientStream> streamHolder(stream);
    streamHolder->CloseStream();

    m_streamManager->CloseStream(std::move(m_streams[stream]));
    m_streams.erase(stream);
  }
}

void CGameClientStreams::SetGameTiming(const game_system_timing& timingInfo)
{
  if (m_streamManager != nullptr)
    m_streamManager->SetVideoFps(static_cast<float>(timingInfo.fps));

  for (const auto& streamEntry : m_streams)
  {
    CGameClientStreamAudio* audioStream = dynamic_cast<CGameClientStreamAudio*>(streamEntry.first);
    if (audioStream != nullptr)
      audioStream->SetSampleRate(timingInfo.sample_rate);
  }
}

bool CGameClientStreams::EnableHardwareRendering(const game_hw_rendering_properties& properties)
{
  if (properties.context_type == GAME_HW_CONTEXT_NONE)
    return false;

  const std::string wanted = CGameClientStreamHwFramebuffer::GetContextName(
      properties.context_type, properties.version_major, properties.version_minor);

  // Refuse before the client commits to rendering this way. It asks this long
  // before the frontend would try to build it a context, and a client told yes
  // wires itself up to callbacks it will then call regardless.
  if (m_streamManager == nullptr || !m_streamManager->HasHardwareRendering())
  {
    CLog::Log(LOGERROR, "GAME: {} is not available on this display stack", wanted);
    m_hwRefusedWanted = wanted;
    m_hwRefusedAvailable.clear();
    return false;
  }

  // Having hardware rendering at all says nothing about which graphics API it
  // speaks: the pool behind it builds OpenGL contexts on a desktop GL build and
  // OpenGL ES contexts on an ES one, and nothing here builds a Vulkan device.
  // Refuse an API this build cannot serve rather than letting the request reach
  // the version check below, which would compare the running OpenGL version
  // against a Vulkan one and report whatever nonsense that produced.
#if defined(HAS_GLES)
  const bool supported = properties.context_type == GAME_HW_CONTEXT_OPENGLES2 ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGLES3 ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGLES_VERSION;
#else
  const bool supported = properties.context_type == GAME_HW_CONTEXT_OPENGL ||
                         properties.context_type == GAME_HW_CONTEXT_OPENGL_CORE;
#endif

  if (!supported)
  {
    // Debug, not error: a client works through the APIs it can use until one is
    // accepted, so a refusal here is the ordinary path. The refusal is recorded,
    // and if nothing is accepted the client tells the user which API it wanted.
    CLog::Log(LOGDEBUG, "GAME: Client asked for {}, which this build does not provide", wanted);
    m_hwRefusedWanted = wanted;
    m_hwRefusedAvailable.clear();
    return false;
  }

  // A client that needs a newer graphics driver than this one has cannot run,
  // and it is worth saying so plainly rather than failing later on a context
  // that could not be created.
  if (properties.version_major != 0)
  {
    unsigned int availableMajor = 0;
    unsigned int availableMinor = 0;
    if (CRenderSystemBase* renderSystem = CServiceBroker::GetRenderSystem())
      renderSystem->GetRenderVersion(availableMajor, availableMinor);

    if (availableMajor != 0 &&
        std::tie(availableMajor, availableMinor) <
            std::tie(properties.version_major, properties.version_minor))
    {
      const std::string available = CGameClientStreamHwFramebuffer::GetContextName(
          properties.context_type, availableMajor, availableMinor);

      CLog::Log(LOGERROR, "GAME: Client needs {}, but this system provides {}", wanted, available);
      m_hwRefusedWanted = wanted;
      m_hwRefusedAvailable = available;
      return false;
    }
  }

  // Log hardware rendering properties for debugging
  CGameClientStreamHwFramebuffer::LogHwProperties(properties);

  // Only OpenGL and OpenGL ES contexts are implemented. Reject anything else
  // here, while the core can still fall back to software rendering, rather
  // than letting it get as far as context_reset() and fail there.
  switch (properties.context_type)
  {
    case GAME_HW_CONTEXT_OPENGL:
    case GAME_HW_CONTEXT_OPENGL_CORE:
    case GAME_HW_CONTEXT_OPENGLES2:
    case GAME_HW_CONTEXT_OPENGLES3:
    case GAME_HW_CONTEXT_OPENGLES_VERSION:
      break;
    default:
    {
      CLog::Log(LOGERROR, "GAME: Hardware rendering context not supported: {}",
                CGameClientStreamHwFramebuffer::GetContextName(
                    properties.context_type, properties.version_major, properties.version_minor));
      return false;
    }
  }

  // Store hardware rendering properties
  m_hwProperties = properties;

  return true;
}

bool CGameClientStreams::BeginClientFrame()
{
  // Nothing to bind for a software client, which is not a failure
  if (m_hwProperties.context_type == GAME_HW_CONTEXT_NONE)
    return true;

  if (m_streamManager == nullptr)
    return false;

  return m_streamManager->BeginClientFrame();
}

void CGameClientStreams::EndClientFrame()
{
  if (m_hwProperties.context_type == GAME_HW_CONTEXT_NONE)
    return;

  if (m_streamManager != nullptr)
    m_streamManager->EndClientFrame();
}

void CGameClientStreams::DestroyHwContext()
{
  if (m_hwProperties.context_type == GAME_HW_CONTEXT_NONE)
    return;

  for (const auto& [stream, retroStream] : m_streams)
  {
    if (auto* hwStream = dynamic_cast<CGameClientStreamHwFramebuffer*>(stream))
      hwStream->DestroyHwContext();
  }
}

game_proc_address_t CGameClientStreams::GetHwProcedureAddress(const char* symbol)
{
  if (m_streamManager != nullptr)
    return m_streamManager->GetHwProcedureAddress(symbol);

  return nullptr;
}

std::unique_ptr<IGameClientStream> CGameClientStreams::CreateStream(
    GAME_STREAM_TYPE streamType) const
{
  std::unique_ptr<IGameClientStream> gameStream;

  switch (streamType)
  {
    case GAME_STREAM_AUDIO:
    {
      gameStream = std::make_unique<CGameClientStreamAudio>(m_gameClient.GetSampleRate());
      break;
    }
    case GAME_STREAM_VIDEO:
    {
      gameStream = std::make_unique<CGameClientStreamVideo>();
      break;
    }
    case GAME_STREAM_HW_FRAMEBUFFER:
    {
      gameStream = std::make_unique<CGameClientStreamHwFramebuffer>(m_gameClient, m_hwProperties);
      break;
    }
    case GAME_STREAM_SW_FRAMEBUFFER:
    {
      gameStream = std::make_unique<CGameClientStreamSwFramebuffer>();
      break;
    }
    default:
      break;
  }

  return gameStream;
}
