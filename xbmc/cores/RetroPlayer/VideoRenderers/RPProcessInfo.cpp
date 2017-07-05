/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPProcessInfo.h"
#include "RPRenderFormats.h"
#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "threads/SingleLock.h"

// Override for platform ports
#if !defined(PLATFORM_OVERRIDE_VP_PROCESSINFO)

CRPProcessInfo* CRPProcessInfo::CreateInstance()
{
  return new CRPProcessInfo();
}

#endif


// base class definitions
CRPProcessInfo::CRPProcessInfo()
{
  ResetVideoInfo();
}

CRPProcessInfo::~CRPProcessInfo()
{

}

void CRPProcessInfo::ResetVideoInfo()
{
  CSingleLock lock(m_videoSection);

  m_videoWidth = 0;
  m_videoHeight = 0;
  m_videoFPS = 0.0;
  m_videoDAR = 0.0;
  m_renderInfo.Reset();

  CServiceBroker::GetDataCacheCore().SetVideoDimensions(m_videoWidth, m_videoHeight);
  CServiceBroker::GetDataCacheCore().SetVideoFps(m_videoFPS);
  CServiceBroker::GetDataCacheCore().SetVideoDAR(m_videoDAR);
}

void CRPProcessInfo::SetVideoDimensions(int width, int height)
{
  CSingleLock lock(m_videoSection);

  m_videoWidth = width;
  m_videoHeight = height;

  CServiceBroker::GetDataCacheCore().SetVideoDimensions(m_videoWidth, m_videoHeight);
}

void CRPProcessInfo::GetVideoDimensions(int &width, int &height)
{
  CSingleLock lock(m_videoSection);

  width = m_videoWidth;
  height = m_videoHeight;
}

void CRPProcessInfo::SetVideoFps(float fps)
{
  CSingleLock lock(m_videoSection);

  m_videoFPS = fps;

  CServiceBroker::GetDataCacheCore().SetVideoFps(m_videoFPS);
}

float CRPProcessInfo::GetVideoFps()
{
  CSingleLock lock(m_videoSection);

  return m_videoFPS;
}

void CRPProcessInfo::SetVideoDAR(float dar)
{
  CSingleLock lock(m_videoSection);

  m_videoDAR = dar;

  CServiceBroker::GetDataCacheCore().SetVideoDAR(m_videoDAR);
}

float CRPProcessInfo::GetVideoDAR()
{
  CSingleLock lock(m_videoSection);

  return m_videoDAR;
}

// player audio info
void CRPProcessInfo::ResetAudioInfo()
{
  CSingleLock lock(m_audioSection);

  m_audioChannels = "unknown";
  m_audioSampleRate = 0;;
  m_audioBitsPerSample = 0;

  CServiceBroker::GetDataCacheCore().SetAudioChannels(m_audioChannels);
  CServiceBroker::GetDataCacheCore().SetAudioSampleRate(m_audioSampleRate);
  CServiceBroker::GetDataCacheCore().SetAudioBitsPerSample(m_audioBitsPerSample);
}

void CRPProcessInfo::SetAudioChannels(const std::string &channels)
{
  CSingleLock lock(m_audioSection);

  m_audioChannels = channels;

  CServiceBroker::GetDataCacheCore().SetAudioChannels(m_audioChannels);
}

std::string CRPProcessInfo::GetAudioChannels()
{
  CSingleLock lock(m_audioSection);

  return m_audioChannels;
}

void CRPProcessInfo::SetAudioSampleRate(int sampleRate)
{
  CSingleLock lock(m_audioSection);

  m_audioSampleRate = sampleRate;

  CServiceBroker::GetDataCacheCore().SetAudioSampleRate(m_audioSampleRate);
}

int CRPProcessInfo::GetAudioSampleRate()
{
  CSingleLock lock(m_audioSection);

  return m_audioSampleRate;
}

void CRPProcessInfo::SetAudioBitsPerSample(int bitsPerSample)
{
  CSingleLock lock(m_audioSection);

  m_audioBitsPerSample = bitsPerSample;

  CServiceBroker::GetDataCacheCore().SetAudioBitsPerSample(m_audioBitsPerSample);
}

int CRPProcessInfo::GetAudioBitsPerSample()
{
  CSingleLock lock(m_audioSection);

  return m_audioBitsPerSample;
}

void CRPProcessInfo::UpdateRenderInfo(CRPRenderInfo &info)
{
  CSingleLock lock(m_renderSection);

  m_renderInfo = info;
}

void CRPProcessInfo::UpdateRenderBuffers(int queued, int discard, int free)
{
  CSingleLock lock(m_renderSection);
  m_renderBufQueued = queued;
  m_renderBufDiscard = discard;
  m_renderBufFree = free;
}

void CRPProcessInfo::GetRenderBuffers(int &queued, int &discard, int &free)
{
  CSingleLock lock(m_renderSection);
  queued = m_renderBufQueued;
  discard = m_renderBufDiscard;
  free = m_renderBufFree;
}
