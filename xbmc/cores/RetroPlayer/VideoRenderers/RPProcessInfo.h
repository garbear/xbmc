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
#pragma once

#include "cores/IPlayer.h"
#include "RPRenderFormats.h"
#include "threads/CriticalSection.h"
#include <list>
#include <string>

class CRPProcessInfo
{
public:
  static CRPProcessInfo* CreateInstance();
  virtual ~CRPProcessInfo();

  // player video info
  void ResetVideoInfo();
  void SetVideoDimensions(int width, int height);
  void GetVideoDimensions(int &width, int &height);
  void SetVideoFps(float fps);
  float GetVideoFps();
  void SetVideoDAR(float dar);
  float GetVideoDAR();

  // player audio info
  void ResetAudioInfo();
  void SetAudioChannels(const std::string &channels);
  std::string GetAudioChannels();
  void SetAudioSampleRate(int sampleRate);
  int GetAudioSampleRate();
  void SetAudioBitsPerSample(int bitsPerSample);
  int GetAudioBitsPerSample();

  // render info
  void UpdateRenderInfo(CRPRenderInfo &info);
  void UpdateRenderBuffers(int queued, int discard, int free);
  void GetRenderBuffers(int &queued, int &discard, int &free);

protected:
  CRPProcessInfo();

  // player video info
  int m_videoWidth;
  int m_videoHeight;
  float m_videoFPS;
  float m_videoDAR;
  CCriticalSection m_videoSection;

  // player audio info
  std::string m_audioChannels;
  int m_audioSampleRate;
  int m_audioBitsPerSample;
  CCriticalSection m_audioSection;

  // render info
  CCriticalSection m_renderSection;
  CRPRenderInfo m_renderInfo;
  int m_renderBufQueued = 0;
  int m_renderBufFree = 0;
  int m_renderBufDiscard = 0;
};
