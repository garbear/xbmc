/*
 *      Copyright (C) 2012-2015 Team XBMC
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

#include "RetroPlayerAudio.h"
#include "RetroPlayerVideo.h"
#include "cores/IPlayer.h"
#include "FileItem.h"
#include "games/GameTypes.h"
#include "threads/Thread.h"
#include "threads/Event.h"

#include <stdint.h>
#include <string>

class CRetroPlayer : public IPlayer, protected CThread
{
public:
  CRetroPlayer(IPlayerCallback& callback);
  virtual ~CRetroPlayer() { CloseFile(); }

  // implementation of IPlayer
  virtual bool  Initialize(TiXmlElement* pConfig) { return true; }
  virtual bool  OpenFile(const CFileItem& file, const CPlayerOptions& options);
  virtual bool  QueueNextFile(const CFileItem &file) { return false; }
  virtual void  OnNothingToQueueNotify() { }
  virtual bool  CloseFile(bool reopen = false);
  virtual bool  IsPlaying() const { return !m_bStop && m_file && m_gameClient; }
  virtual bool  CanPause() { return true; }
  virtual void  Pause();
  virtual bool  IsPaused() const { return m_playSpeed == 0; }
  virtual bool  HasVideo() const { return true; }
  virtual bool  HasAudio() const { return true; }
  virtual bool  IsPassthrough() const { return false; }
  virtual bool  CanSeek() { return true; }
  virtual void  Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false);
  virtual bool  SeekScene(bool bPlus = true) { return false; }
  virtual void  SeekPercentage(float fPercent = 0);
  virtual float GetPercentage();
  virtual float GetCachePercentage() { return 0; }
  virtual void  SetMute(bool bOnOff) { }
  virtual void  SetVolume(float volume) { }
  virtual bool  ControlsVolume() { return false; }
  virtual void  SetDynamicRangeCompression(long drc) { }
  virtual void  GetAudioInfo(std::string& strAudioInfo)     { strAudioInfo   = "CRetroPlayer:GetAudioInfo"; }
  virtual void  GetVideoInfo(std::string& strVideoInfo)     { strVideoInfo   = "CRetroPlayer:GetVideoInfo"; }
  virtual void  GetGeneralInfo(std::string& strGeneralInfo) { strGeneralInfo = "CRetroPlayer:GetGeneralInfo"; }
  virtual bool  CanRecord() { return false; }
  virtual bool  IsRecording() { return false; }
  virtual bool  Record(bool bOnOff) { return false; }

  virtual void  SetAVDelay(float fValue = 0.0f) { }
  virtual float GetAVDelay()                    { return 0.0f; }

  virtual void  SetSubTitleDelay(float fValue = 0.0f) { }
  virtual float GetSubTitleDelay()     { return 0.0f; }
  virtual int   GetSubtitleCount()     { return 0; }
  virtual int   GetSubtitle()          { return -1; }
  virtual void  GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info) { }
  virtual void  SetSubtitle(int iStream) { }
  virtual bool  GetSubtitleVisible() { return false; }
  virtual void  SetSubtitleVisible(bool bVisible) { }
  virtual void  AddSubtitle(const std::string& strSubPath) { }

  virtual int   GetAudioStreamCount()  { return 0; }
  virtual int   GetAudioStream()       { return -1; }
  virtual void  SetAudioStream(int iStream) { }
  virtual void  GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info) { }

  virtual TextCacheStruct_t* GetTeletextCache() { return NULL; }
  virtual void               LoadPage(int p, int sp, unsigned char* buffer) { }

  virtual int   GetChapterCount()                           { return 0; }
  virtual int   GetChapter()                                { return -1; }
  virtual void  GetChapterName(std::string& strChapterName) { }
  virtual int   SeekChapter(int iChapter)                   { return -1; }

  virtual float   GetActualFPS() { return 0.0f; }
  virtual void    SeekTime(int64_t iTime = 0);
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();
  virtual void    GetVideoStreamInfo(SPlayerVideoStreamInfo &info) { }
  virtual int     GetSourceBitrate() { return 0; }
  virtual bool    GetStreamDetails(CStreamDetails &details) { return false; }
  virtual void    ToFFRW(int iSpeed = 0);
  virtual bool    SkipNext() {return false; }

  virtual bool  IsCaching() const { return false; }
  virtual int   GetCacheLevel() const { return -1; }

  virtual bool  IsInMenu() const { return false; }
  virtual bool  HasMenu() { return false; }

  virtual void  DoAudioWork() { }
  virtual bool  OnAction(const CAction &action) { return false; }

  virtual std::string GetPlayerState() { return ""; }
  virtual bool        SetPlayerState(const std::string& state) { return false; }

  virtual std::string GetPlayingTitle() { return ""; }

  virtual bool SwitchChannel(PVR::CPVRChannel &channel) { return false; }

  virtual void OMXGetRenderFeatures(std::vector<int> &renderFeatures)         { }
  virtual void OMXGetDeinterlaceMethods(std::vector<int> &deinterlaceMethods) { }
  virtual void OMXGetDeinterlaceModes(std::vector<int> &deinterlaceModes)     { }
  virtual void OMXGetScalingMethods(std::vector<int> &scalingMethods)         { }

  virtual void GetAudioCapabilities(std::vector<int> &audioCaps)  { audioCaps.assign(1, IPC_AUD_ALL); }
  virtual void GetSubtitleCapabilities(std::vector<int> &subCaps) { subCaps.assign(1, IPC_SUBS_ALL); }

  // Game API
  void VideoFrame(const uint8_t* data, unsigned int size, unsigned int width, unsigned int height, AVPixelFormat format) { m_video.VideoFrame(data, size, width, height, format); }
  bool VideoFrameH264(const uint8_t* data, unsigned int size, unsigned int width, unsigned int height) { return m_video.VideoFrameH264(data, size, width, height); }
  void AudioFrames(const uint8_t* data, unsigned int size, unsigned int frames, AEDataFormat format) { m_audio.AudioFrames(data, size, frames, format); }

protected:
  virtual void Process();

private:
  /**
   * Dump game information (if any) to the debug log.
   */
  void PrintGameInfo(const CFileItem &file) const;

  /**
   * Create the audio component. Chooses a compatible samplerate and returns
   * a multiplier representing the framerate adjustment factor, allowing us to
   * sync the video clock to the audio.
   * @param  samplerate - the game client's reported audio sample rate
   * @return the framerate multiplier (chosen samplerate / specified samplerate)
   *         or 1.0 if no audio.
   */
  void CreateAudio(double samplerate);

  CRetroPlayerVideo    m_video;
  CRetroPlayerAudio    m_audio;

  CFileItemPtr         m_file;
  GAME::GameClientPtr  m_gameClient;

  CPlayerOptions       m_PlayerOptions;
  int                  m_playSpeed; // Normal play speed is PLAYSPEED_NORMAL (1000)
  double               m_audioSpeedFactor; // Factor by which the audio is sped up
  CEvent               m_pauseEvent;
  CCriticalSection     m_critSection; // For synchronization of Open() and Close() calls

  unsigned int m_samplerate;
};
