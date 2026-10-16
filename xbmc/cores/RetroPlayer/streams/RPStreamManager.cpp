/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPStreamManager.h"

#include "IRetroPlayerStream.h"
#include "RetroPlayerAudio.h"
#include "RetroPlayerRendering.h"
#include "RetroPlayerVideo.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"

using namespace KODI;
using namespace RETRO;

CRPStreamManager::CRPStreamManager(CRPRenderManager& renderManager, CRPProcessInfo& processInfo)
  : m_renderManager(renderManager),
    m_processInfo(processInfo)
{
  // Visual streams can overlap, so shared rendering resources belong to the session.
  m_renderManager.Initialize();
}

CRPStreamManager::~CRPStreamManager()
{
  m_renderManager.Deinitialize();
}

void CRPStreamManager::EnableAudio(bool bEnable)
{
  if (m_audioStream != nullptr)
    m_audioStream->Enable(bEnable);
}

void CRPStreamManager::SuppressAudio(bool bSuppress)
{
  m_audioSuppressed = bSuppress;

  if (m_audioStream != nullptr)
    m_audioStream->Suppress(bSuppress);
}

void CRPStreamManager::EnableVideo(bool bEnable)
{
  m_videoEnabled = bEnable;

  if (m_videoStream != nullptr)
    m_videoStream->Enable(bEnable);

  if (m_renderingStream != nullptr)
    m_renderingStream->Enable(bEnable);
}

StreamPtr CRPStreamManager::CreateStream(StreamType streamType)
{
  switch (streamType)
  {
    case StreamType::AUDIO:
    {
      auto audioStream = std::make_unique<CRetroPlayerAudio>(m_processInfo);
      audioStream->Suppress(m_audioSuppressed);

      // Save pointer to audio stream
      m_audioStream = audioStream.get();

      return StreamPtr{audioStream.release()};
    }
    case StreamType::VIDEO:
    case StreamType::SW_BUFFER:
    {
      auto videoStream = std::make_unique<CRetroPlayerVideo>(m_renderManager, m_processInfo);
      videoStream->Enable(m_videoEnabled);

      m_videoStream = videoStream.get();

      return StreamPtr{videoStream.release()};
    }
    case StreamType::HW_BUFFER:
    {
      auto renderingStream =
          std::make_unique<CRetroPlayerRendering>(m_renderManager, m_processInfo);
      renderingStream->Enable(m_videoEnabled);

      m_renderingStream = renderingStream.get();

      return StreamPtr{renderingStream.release()};
    }
    default:
      break;
  }

  return StreamPtr();
}

void CRPStreamManager::CloseStream(StreamPtr stream)
{
  if (stream)
  {
    if (stream.get() == m_audioStream)
      m_audioStream = nullptr;
    else if (stream.get() == m_videoStream)
      m_videoStream = nullptr;
    else if (stream.get() == m_renderingStream)
      m_renderingStream = nullptr;

    stream->CloseStream();
  }
}

void CRPStreamManager::SetVideoFps(float fps)
{
  m_processInfo.SetVideoFps(fps);
}

bool CRPStreamManager::BeginClientFrame()
{
  return m_renderManager.BeginClientFrame();
}

void CRPStreamManager::EndClientFrame()
{
  m_renderManager.EndClientFrame();
}

HwProcedureAddress CRPStreamManager::GetHwProcedureAddress(const char* symbol)
{
  return m_processInfo.GetHwProcedureAddress(symbol);
}

bool CRPStreamManager::HasHardwareRendering() const
{
  return m_processInfo.HasHardwareRendering();
}
