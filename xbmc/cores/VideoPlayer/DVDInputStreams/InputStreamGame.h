/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "DVDInputStream.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "games/addons/GameClient.h"

#include <stdint.h>
#include <vector>

class CInputStreamGame : public CDVDInputStream
{
public:
  CInputStreamGame(const CFileItem &fileitem, const GAME::GameClientPtr &gameClient);

  virtual ~CInputStreamGame();

  // implementation of CDVDInputStream
  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t *buf, int buf_size) override { return -1; } // Can't read arbitrary bytes, must read full packet from demuxer
  virtual int64_t Seek(int64_t offset, int whence) override { return -1; }
  virtual bool Pause(double dTime) override { return false; } // Must pause demuxer
  virtual int64_t GetLength() override { return -1; }
  virtual ENextStream NextStream();
  virtual bool CanSeek() override { return false; }
  virtual bool CanPause() override { return false; }
  virtual bool IsEOF() override { return false; } // Read until demuxer returns no data

  // interfaces
  virtual IDisplayTime* GetIDisplayTime() override { return m_pDisplayTime; }
  virtual IPosTime* GetIPosTime() override { return m_pPosTime; }
  virtual IDemux* GetIDemux() override { return m_pDemux; }

private:
  const GAME::GameClientPtr m_gameClient;
  IDisplayTime* m_pDisplayTime;
  IPosTime* m_pPosTime;
  IDemux* m_pDemux;
};

class CInputStreamGameDisplayTime : public CDVDInputStream::IDisplayTime
{
public:
  CInputStreamGameDisplayTime(const GAME::GameClientPtr &gameClient);

  virtual ~CInputStreamGameDisplayTime() { }

  // implementation of CDVDInputStream::IDisplayTime
  virtual int GetTotalTime() override;
  virtual int GetTime() override;

private:
  const GAME::GameClientPtr m_gameClient;
};

class CInputStreamGamePosTime : public CDVDInputStream::IPosTime
{
public:
  CInputStreamGamePosTime(const GAME::GameClientPtr &gameClient);

  virtual ~CInputStreamGamePosTime() { }

  // implementation of CDVDInputStream::IPosTime
  virtual bool PosTime(int ms) override;

private:
  const GAME::GameClientPtr m_gameClient;
};

class CInputStreamGameDemux : public CDVDInputStream::IDemux,
                              public GAME::IGameDemuxCallback
{
public:
  CInputStreamGameDemux(const GAME::GameClientPtr &gameClient);

  virtual ~CInputStreamGameDemux();

  // implementation of CDVDInputStream::IDemux
  virtual bool OpenDemux() override;
  virtual DemuxPacket* ReadDemux() override;
  virtual CDemuxStream* GetStream(int iStreamId) const override;
  virtual std::vector<CDemuxStream*> GetStreams() const override;
  virtual void EnableStream(int iStreamId, bool enable) override;
  virtual int GetNrOfStreams() const override;
  virtual void SetSpeed(int iSpeed) override;
  virtual bool SeekTime(int time, bool backward = false, double* startpts = NULL) override;
  virtual void AbortDemux() override;
  virtual void FlushDemux() override;

  // implementation of IGameDemuxCallback
  virtual void OpenVideoStream(AVCodecID codec, AVPixelFormat pixfmt, unsigned int width, unsigned int height);
  virtual void CloseVideoStream();
  virtual void OpenAudioStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout);
  virtual void CloseAudioStream();

private:
  const GAME::GameClientPtr m_gameClient;

  CDemuxStreamVideo* m_videoStream;
  CDemuxStreamAudio* m_audioStream;

  std::vector<uint8_t> m_buffer;
};
