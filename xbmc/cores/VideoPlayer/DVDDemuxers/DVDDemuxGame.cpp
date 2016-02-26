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

#include "DVDDemuxGame.h"
#include "DVDDemuxPacket.h"

CDVDDemuxGame::CDVDDemuxGame(CDVDInputStream *pInput) :
  m_pInput(pInput),
  m_IDemux(nullptr)
{
}

CDVDDemuxGame::~CDVDDemuxGame()
{
  Dispose();
}

bool CDVDDemuxGame::Open()
{
  Abort();

  m_IDemux = m_pInput->GetIDemux();
  if (!m_IDemux)
    return false;

  if (!m_IDemux->OpenDemux())
  {
    m_IDemux = nullptr;
    return false;
  }

  return true;
}

void CDVDDemuxGame::Dispose()
{
  m_IDemux = nullptr;
}

void CDVDDemuxGame::Reset()
{
  Open();
}

void CDVDDemuxGame::Abort()
{
  if (m_IDemux)
    m_IDemux->AbortDemux();
}

void CDVDDemuxGame::Flush()
{
  if (m_IDemux)
    m_IDemux->FlushDemux();
}

DemuxPacket* CDVDDemuxGame::Read()
{
  if (m_IDemux)
    return m_IDemux->ReadDemux();

  return nullptr;
}

bool CDVDDemuxGame::SeekTime(int timems, bool backwards, double *startpts)
{
  if (m_IDemux)
    return m_IDemux->SeekTime(timems, backwards, startpts);

  return false;
}

void CDVDDemuxGame::SetSpeed(int speed)
{
  if (m_IDemux)
    m_IDemux->SetSpeed(speed);
}

int CDVDDemuxGame::GetStreamLength()
{
  return 0; // TODO
}

std::vector<CDemuxStream*> CDVDDemuxGame::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  if (m_IDemux)
    streams = m_IDemux->GetStreams();

  return streams;
}

CDemuxStream* CDVDDemuxGame::GetStream(int iStreamId) const
{
  if (m_IDemux)
    return m_IDemux->GetStream(iStreamId);

  return nullptr;
}

int CDVDDemuxGame::GetNrOfStreams() const
{
  if (m_IDemux)
    return m_IDemux->GetNrOfStreams();

  return 0;
}

std::string CDVDDemuxGame::GetFileName()
{
  if (m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxGame::GetStreamCodecName(int iStreamId)
{
  std::string strName;

  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    switch (stream->codec)
    {
    case AV_CODEC_ID_AC3:
      strName = "ac3";
      break;
    case AV_CODEC_ID_MP2:
      strName = "mpeg2audio";
      break;
    case AV_CODEC_ID_AAC:
      strName = "aac";
      break;
    case AV_CODEC_ID_DTS:
      strName = "dts";
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      strName = "mpeg2video";
      break;
    case AV_CODEC_ID_H264:
      strName = "h264";
      break;
    case AV_CODEC_ID_EAC3:
      strName = "eac3";
      break;
    case AV_CODEC_ID_HEVC:
      strName = "hevc";
      break;
    default:
      break;
    }
  }
  return strName;
}

void CDVDDemuxGame::EnableStream(int id, bool enable)
{
  if (m_IDemux)
    m_IDemux->EnableStream(id, enable);
}
