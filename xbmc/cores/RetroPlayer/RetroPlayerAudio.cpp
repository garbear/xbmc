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

#include "RetroPlayerAudio.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

using namespace GAME;

CRetroPlayerAudio::CRetroPlayerAudio() :
  m_pAudioStream(NULL),
  m_bAudioEnabled(true)
{
}

unsigned int CRetroPlayerAudio::NormalizeSamplerate(unsigned int samplerate) const
{
  // TODO: List comes from AESinkALSA.cpp many moons ago
  static unsigned int sampleRateList[] = { 5512, 8000, 11025, 16000, 22050, 32000, 44100, 48000, 0 };

  for (unsigned int *rate = sampleRateList; ; rate++)
  {
    const unsigned int thisValue = *rate;
    const unsigned int nextValue = *(rate + 1);

    if (nextValue == 0)
    {
      // Reached the end of our list
      return thisValue;
    }

    if (samplerate < (thisValue + nextValue) / 2)
    {
      // samplerate is between this rate and the next, so use this rate
      return thisValue;
    }
  }

  return samplerate; // Shouldn't happen
}

bool CRetroPlayerAudio::OpenPCMStream(AEDataFormat format, unsigned int samplerate, const CAEChannelInfo& channelLayout)
{
  if (m_pAudioStream != NULL)
    CloseStream();

  CLog::Log(LOGINFO, "RetroPlayerAudio: Creating audio stream, sample rate = %d", samplerate);

  if (NormalizeSamplerate(samplerate) != samplerate)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Resampling to %d not supported", NormalizeSamplerate(samplerate));
    return false;
  }

  AEAudioFormat audioFormat;
  audioFormat.m_dataFormat = format;
  audioFormat.m_sampleRate = samplerate; // TODO: Normalize samplerate
  audioFormat.m_channelLayout = channelLayout;
  m_pAudioStream = CAEFactory::MakeStream(audioFormat);

  if (!m_pAudioStream)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Failed to create audio stream");
    return false;
  }

  return true;
}

bool CRetroPlayerAudio::OpenEncodedStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout)
{
  /* TODO
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
  */
  return false;
}

void CRetroPlayerAudio::AddData(const uint8_t* data, unsigned int size)
{
  if (m_pAudioStream && m_bAudioEnabled)
  {
    const unsigned int frameSize = m_pAudioStream->GetChannelCount() * (CAEUtil::DataFormatToBits(m_pAudioStream->GetDataFormat()) >> 3);
    m_pAudioStream->AddData(&data, 0, size / frameSize);
  }
  // TODO: Encoded data
}

void CRetroPlayerAudio::CloseStream()
{
  if (m_pAudioStream)
  {
    CAEFactory::FreeStream(m_pAudioStream);
    m_pAudioStream = NULL;
  }
}
