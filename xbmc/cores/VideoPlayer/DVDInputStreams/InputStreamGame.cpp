/*
 *      Copyright (C) 2016 Team Kodi
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

#include "InputStreamGame.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "games/addons/GameClient.h"

// --- CInputStreamGame --------------------------------------------------------

CInputStreamGame::CInputStreamGame(const CFileItem &fileitem, const GAME::GameClientPtr &gameClient) :
  CDVDInputStream(DVDSTREAM_TYPE_GAME, fileitem),
  m_gameClient(gameClient),
  m_pDisplayTime(nullptr),
  m_pPosTime(nullptr),
  m_pDemux(new CInputStreamGameDemux(gameClient))
{
  // initialize CDVDInputStream
  SetRealtime(true);
}

CInputStreamGame::~CInputStreamGame()
{
  Close();

  delete m_pDisplayTime;
  delete m_pPosTime;
  delete m_pDemux;
}

bool CInputStreamGame::Open()
{
  bool bSuccess = false;

  if (m_gameClient && m_gameClient->Initialize())
  {
    /* TODO
    bool hasDisplayTime = m_gameClient->CanSeek(); // TODO
    bool hasPosTime = m_gameClient->CanSeek(); // TODO

    if (hasDisplayTime)
    {
      delete m_pDisplayTime;
      m_pDisplayTime = new CInputStreamGameDisplayTime(m_gameClient);
    }

    if (hasPosTime)
    {
      delete m_pPosTime;
      m_pPosTime = new CInputStreamGamePosTime(m_gameClient);
    }
    */

    bSuccess = m_gameClient->OpenFile(m_item, static_cast<CInputStreamGameDemux*>(m_pDemux));

    if (!bSuccess)
      m_gameClient->Destroy();
  }

  return bSuccess;
}

void CInputStreamGame::Close()
{
  m_gameClient->CloseFile();
  m_gameClient->Destroy();
}

CDVDInputStream::ENextStream CInputStreamGame::NextStream()
{
  bool bIsPlaying = m_gameClient->IsPlaying();
  return bIsPlaying ? NEXTSTREAM_RETRY : NEXTSTREAM_NONE;
}

// --- CInputStreamGameDisplayTime ---------------------------------------------

CInputStreamGameDisplayTime::CInputStreamGameDisplayTime(const GAME::GameClientPtr &gameClient) :
  m_gameClient(gameClient)
{
}

int CInputStreamGameDisplayTime::GetTotalTime()
{
  return 0; // TODO
}

int CInputStreamGameDisplayTime::GetTime()
{
  return 0; // TODO
}

// --- CInputStreamGamePosTime -------------------------------------------------

CInputStreamGamePosTime::CInputStreamGamePosTime(const GAME::GameClientPtr &gameClient) :
  m_gameClient(gameClient)
{
}

bool CInputStreamGamePosTime::PosTime(int ms)
{
  return false; // TODO
}

// --- CInputStreamGameDemux ---------------------------------------------------

CInputStreamGameDemux::CInputStreamGameDemux(const GAME::GameClientPtr &gameClient) :
  m_gameClient(gameClient),
  m_videoStream(nullptr),
  m_audioStream(nullptr)
{
}

CInputStreamGameDemux::~CInputStreamGameDemux()
{
  delete m_videoStream;
  delete m_audioStream;
}

bool CInputStreamGameDemux::OpenDemux()
{
  return true;
}

DemuxPacket* CInputStreamGameDemux::ReadDemux()
{
  DemuxPacket* packet = m_gameClient->ReadDemux();
  if (packet)
  {
    // Check if packet should be ignored
    bool bIgnore = false;

    switch (packet->iStreamId)
    {
    case GAME_STREAM_VIDEO_ID:
      if (!m_videoStream || m_videoStream->disabled)
        bIgnore = true;
      break;
    case GAME_STREAM_AUDIO_ID:
      if (!m_audioStream || m_audioStream->disabled)
        bIgnore = true;
      break;
    }

    if (bIgnore)
    {
      CDVDDemuxUtils::FreeDemuxPacket(packet);
      packet = NULL;
    }
  }

  return packet;
}

CDemuxStream* CInputStreamGameDemux::GetStream(int iStreamId) const
{
  switch (iStreamId)
  {
    case GAME_STREAM_VIDEO_ID:
      return m_videoStream;
    case GAME_STREAM_AUDIO_ID:
      return m_audioStream;
    default:
      break;
  }

  return nullptr;
}

std::vector<CDemuxStream*> CInputStreamGameDemux::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  if (m_videoStream)
    streams.push_back(m_videoStream);
  if (m_audioStream)
    streams.push_back(m_audioStream);

  return streams;
}

void CInputStreamGameDemux::EnableStream(int iStreamId, bool enable)
{
  switch (iStreamId)
  {
    case GAME_STREAM_VIDEO_ID:
      if (m_videoStream)
        m_videoStream->disabled = !enable;
      break;
    case GAME_STREAM_AUDIO_ID:
      if (m_audioStream)
        m_audioStream->disabled = !enable;
      break;
    default:
      break;
  }
}

int CInputStreamGameDemux::GetNrOfStreams() const
{
  unsigned int count = 0;

  if (m_videoStream)
    count++;
  if (m_audioStream)
    count++;

  return count;
}

void CInputStreamGameDemux::SetSpeed(int iSpeed)
{
  const double speed = static_cast<double>(iSpeed); // TODO: Correct conversion?
  //m_gameClient->SetSpeed(speed); // TODO
}

bool CInputStreamGameDemux::SeekTime(int time, bool backward, double* startpts)
{
  /* TODO
  if (m_gameClient->CanSeek())
  {
    // TODO
    if (startpts)
      *startpts = DVD_NOPTS_VALUE;
    return true;
  }
  */

  return false; // m_gameClient->SeekTime(time, backward, startpts); // TODO
}

void CInputStreamGameDemux::AbortDemux()
{
  m_gameClient->CloseFile();
}

void CInputStreamGameDemux::FlushDemux()
{
  m_gameClient->FlushDemux();
}

void CInputStreamGameDemux::OpenVideoStream(AVCodecID codec,
                                            AVPixelFormat pixfmt,
                                            unsigned int width,
                                            unsigned int height)
{
  if (!m_videoStream)
    m_videoStream = new CDemuxStreamVideo;
  else
    m_videoStream->changes++;

  // Stream
  m_videoStream->uniqueId = GAME_STREAM_VIDEO_ID;
  m_videoStream->codec = codec;
  m_videoStream->codec_fourcc = 0; // TODO
  m_videoStream->type = STREAM_VIDEO;
  m_videoStream->source = STREAM_SOURCE_DEMUX;
  m_videoStream->realtime = true;
  m_videoStream->disabled = false;

  // Video
  m_videoStream->pixfmt = pixfmt;
  m_videoStream->iHeight = height;
  m_videoStream->iWidth = width;
}

void CInputStreamGameDemux::CloseVideoStream()
{
  delete m_videoStream;
  m_videoStream = nullptr;
}

void CInputStreamGameDemux::OpenAudioStream(AVCodecID codec,
                                            unsigned int samplerate,
                                            const CAEChannelInfo& channelLayout)
{
  if (!m_audioStream)
    m_audioStream = new CDemuxStreamAudio;
  else
    m_audioStream->changes++;

  // Stream
  m_audioStream->uniqueId = GAME_STREAM_AUDIO_ID;
  m_audioStream->codec = codec;
  m_audioStream->codec_fourcc = 0; // TODO
  m_audioStream->type = STREAM_AUDIO;
  m_audioStream->source = STREAM_SOURCE_DEMUX;
  m_audioStream->realtime = true;
  m_audioStream->disabled = false;

  // Audio
  m_audioStream->iChannels = channelLayout.Count();
  m_audioStream->iSampleRate = samplerate;
  m_audioStream->iChannelLayout = CAEUtil::GetAVChannelLayout(channelLayout);
}

void CInputStreamGameDemux::CloseAudioStream()
{
  delete m_audioStream;
  m_audioStream = nullptr;
}
