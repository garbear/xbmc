/*
 *      Copyright (C) 2017 Unique Digital Ltd.
 *      Copyright (C) 2017 Team Kodi
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

#include "MuxerFFmpeg.h"
#include "MediaCache.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/Interface/Addon/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/Addon/TimingConstants.h"
#include "cores/FFmpeg.h"
#include "utils/log.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/error.h"
#include "libavutil/mem.h"
}

#include <algorithm>
#include <math.h>

using namespace KODI;
using namespace MEDIA;

//! @todo
#ifndef FFMPEG_FILE_BUFFER_SIZE
#  define FFMPEG_FILE_BUFFER_SIZE   32768 // default reading size for ffmpeg
#endif

static int InterruptCallback(void *ctx)
{
  CMuxerFFmpeg *muxer = static_cast<CMuxerFFmpeg*>(ctx);
  if (!muxer)
    return 1;

  if (muxer->Aborted())
    return 1;

  return 0;
}

static int WritePacket(void *handle, uint8_t *buf, int size)
{
  if (InterruptCallback(handle))
    return AVERROR_EXIT;

  CMuxerFFmpeg *muxer = static_cast<CMuxerFFmpeg*>(handle);
  if (!muxer)
    return AVERROR_EXIT;

  muxer->WriteOutputPacket(buf, size);

  return 0;
}

static int64_t SeekPosition(void *handle, int64_t offset, int whence)
{
  if ((whence & AVSEEK_SIZE) != 0)
    return -1; // Unsupported until muxer can report file length

  if (InterruptCallback(handle))
    return AVERROR_EXIT;

  CMuxerFFmpeg *muxer = static_cast<CMuxerFFmpeg*>(handle);
  if (!muxer)
    return AVERROR_EXIT;

  int64_t newPos = muxer->Seek(offset, whence & ~AVSEEK_FORCE);
  if (newPos < 0)
  {
    CLog::Log(LOGDEBUG, "Can't seek muxed file to offset %d (%d)", offset, whence);
    return AVERROR_EXIT;
  }

  return newPos;
}

void CMuxerFFmpeg::delete_format_context::operator()(AVFormatContext *formatContext) const
{
  /*
  if (formatContext && formatContext->pb)
  {
    av_free(&formatContext->pb->buffer);
    av_free(&formatContext->pb);
  }
  */
  avformat_free_context(formatContext);
}

CMuxerFFmpeg::CMuxerFFmpeg(CMediaCache *callback, int inputBlockSize /* = 0 */) :
  m_callback(callback),
  m_inputBlockSize(inputBlockSize),
  m_bAborted(false)
{
}

CMuxerFFmpeg::~CMuxerFFmpeg()
{
  Close();
}

bool CMuxerFFmpeg::Open(const std::vector<CDemuxStream*>& streams)
{
  if (m_callback == nullptr)
    return false;

  // Reset aborted flag
  m_bAborted = false;

  // Set format
  AVOutputFormat *format = av_guess_format(nullptr, m_callback->GetLocalPath().c_str(), nullptr);
  if (format == nullptr)
  {
    CLog::LogF(LOGERROR, "Could not deduce output format from file extension: %s", m_callback->GetLocalPath().c_str());
    return false;
  }

  //format->flags |= AVFMT_NOFILE; //! @todo

  CLog::Log(LOGDEBUG, "Muxing using %s format into %s", format->name, m_callback->GetLocalPath().c_str());

  // Create context
  AVFormatContext *formatContext = nullptr;
  if (avformat_alloc_output_context2(&formatContext, format, nullptr, m_callback->GetLocalPath().c_str()) != 0)
  {
    CLog::LogF(LOGERROR, "Could not alloc output context for %s", m_callback->GetLocalPath().c_str());
    return false;
  }
  m_formatContext.reset(formatContext);

  // Set I/O context
  unsigned char *buffer = static_cast<unsigned char*>(av_malloc(FFMPEG_FILE_BUFFER_SIZE));
  m_formatContext->pb = avio_alloc_context(buffer, FFMPEG_FILE_BUFFER_SIZE, AVIO_FLAG_WRITE, this, nullptr, WritePacket, SeekPosition);
  if (m_formatContext->pb == nullptr)
  {
    av_free(&buffer);
    CLog::LogF(LOGERROR, "Failed to allocate ByteIOContext");
    return false;
  }
  m_formatContext->pb->direct = 1;
  const AVIOInterruptCB int_cb = { InterruptCallback, this };
  m_formatContext->interrupt_callback = int_cb;

  // Add streams
  for (const auto* stream : streams)
  {
    if (!AddStream(stream))
      return false;
  }

  if (m_formatContext->nb_streams == 0)
  {
    CLog::LogF(LOGERROR, "Input contains no valid streams");
    return false;
  }

  // Write header
  int ret = avformat_write_header(m_formatContext.get(), nullptr);
  if (ret < 0)
  {
    char errorDesc[AV_ERROR_MAX_STRING_SIZE] = { };
    av_strerror(ret, errorDesc, sizeof(errorDesc));
    CLog::LogF(LOGERROR, "Failed to write header: %s", errorDesc);
    return false;
  }

  return true;
}

bool CMuxerFFmpeg::AddStream(const CDemuxStream *stream)
{
  std::vector<StreamType> validStreams = { STREAM_VIDEO, STREAM_AUDIO, STREAM_SUBTITLE };
  if (std::find(validStreams.begin(), validStreams.end(), stream->type) == validStreams.end())
  {
    // Invalid stream, continue
    CLog::LogF(LOGDEBUG, "Skipping stream %d with invalid type %d", stream->uniqueId, stream->type);
    return true;
  }

  AVCodec *codec = nullptr;// avcodec_find_encoder(stream->codec);
  AVStream *ffmpegStream = avformat_new_stream(m_formatContext.get(), codec);
  if (ffmpegStream == nullptr)
  {
    if (codec && codec->name)
      CLog::LogF(LOGERROR, "Could not alloc stream with codec: %s", codec->name);
    else
      CLog::LogF(LOGERROR, "Could not alloc stream with unknown codec");
    return false;
  }
  // This is the MPEG-TS stream identifier + 1; 0 means unknown
  ffmpegStream->stream_identifier = stream->uniqueId + 1;

  AVCodecParameters *codecParams = ffmpegStream->codecpar;

  // Set codec ID
  codecParams->codec_id = stream->codec;

  // Set remaining properties based on type
  switch (stream->type)
  {
  case STREAM_VIDEO:
  {
    CLog::LogF(LOGDEBUG, "Adding video stream %d with MPEG-TS ID %d", stream->uniqueId, ffmpegStream->stream_identifier);

    const CDemuxStreamVideo *video = static_cast<const CDemuxStreamVideo*>(stream);

#if defined(AVFORMAT_HAS_STREAM_GET_R_FRAME_RATE)
    av_stream_set_r_frame_rate(ffmpegStream, AVRational{ video->iFpsRate, video->iFpsScale });
#else
    ffmpegStream->r_frame_rate.num = video->iFpsRate;
    ffmpegStream->r_frame_rate.den = video->iFpsScale;
#endif
    ffmpegStream->time_base.num = video->iFpsScale;
    ffmpegStream->time_base.den = video->iFpsRate;

    codecParams->codec_type = AVMEDIA_TYPE_VIDEO;
    codecParams->format = static_cast<int>(video->format);
    codecParams->bit_rate = video->iBitRate;
    codecParams->width = video->iWidth;
    codecParams->height = video->iHeight;
    codecParams->bits_per_coded_sample = video->iBitsPerPixel;
    codecParams->sample_aspect_ratio.num = lrint(video->fAspect * video->iHeight);
    codecParams->sample_aspect_ratio.den = video->iHeight;

    break;
  }
  case STREAM_AUDIO:
  {
    CLog::LogF(LOGDEBUG, "Adding audio stream %d with MPEG-TS ID %d", stream->uniqueId, ffmpegStream->stream_identifier);

    const CDemuxStreamAudio *audio = static_cast<const CDemuxStreamAudio*>(stream);

    codecParams->codec_type = AVMEDIA_TYPE_AUDIO;
    codecParams->format = static_cast<int>(audio->format);
    codecParams->bit_rate = audio->iBitRate;
    codecParams->sample_rate = audio->iSampleRate;
    codecParams->channels = audio->iChannels;
    codecParams->block_align = audio->iBlockAlign;
    codecParams->bits_per_coded_sample = audio->iBitsPerSample;
    codecParams->channel_layout = audio->iChannelLayout;
    codecParams->bits_per_coded_sample = audio->iBitsPerSample;

    break;
  }
  case STREAM_SUBTITLE:
  {
    CLog::LogF(LOGDEBUG, "Adding subtitle stream %d with MPEG-TS ID %d", stream->uniqueId, ffmpegStream->stream_identifier);
    codecParams->codec_type = AVMEDIA_TYPE_SUBTITLE;
    break;
  }
  case STREAM_DATA:
  {
    CLog::LogF(LOGDEBUG, "Adding data stream %d with MPEG-TS ID %d", stream->uniqueId, ffmpegStream->stream_identifier);
    codecParams->codec_type = AVMEDIA_TYPE_DATA;
    break;
  }
  case STREAM_TELETEXT:
  case STREAM_RADIO_RDS:
  default:
  {
    CLog::LogF(LOGERROR, "Invalid stream type %d for stream with ID %d", stream->type, stream->uniqueId);
    codecParams->codec_type = AVMEDIA_TYPE_UNKNOWN;
    break;
  }
  }

  return true;
}

void CMuxerFFmpeg::Close()
{
  if (m_formatContext)
    av_write_trailer(m_formatContext.get());

  m_formatContext.reset();
}

bool CMuxerFFmpeg::Write(const DemuxPacket& packet, CDemuxStream *stream)
{
  if (m_formatContext->nb_streams == 0)
    return false;

  bool bIsValid = false;

  for (unsigned int i = 0; i < m_formatContext->nb_streams; i++)
  {
    if (m_formatContext->streams[i]->stream_identifier == stream->uniqueId + 1)
    {
      bIsValid = true;
      break;
    }
  }

  // Skip packet
  if (!bIsValid)
    return true;

  // Find stream index by MPEG TS identifier
  int streamIndex = -1;
  for (unsigned int i = 0; i < m_formatContext->nb_streams; i++)
  {
    if (m_formatContext->streams[i]->stream_identifier == stream->uniqueId + 1)
    {
      streamIndex = i;
      break;
    }
  }

  if (streamIndex < 0)
  {
    CLog::LogF(LOGERROR, "MPEG TS identifier (%d) not found", stream->uniqueId + 1);
    return false;
  }

  AVPacket pkt;
  av_init_packet(&pkt);

  pkt.stream_index = streamIndex;
  pkt.data = packet.pData;
  pkt.size = packet.iSize;
  pkt.dts = (packet.dts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : static_cast<int64_t>(packet.dts / DVD_TIME_BASE * AV_TIME_BASE);
  pkt.pts = (packet.pts == DVD_NOPTS_VALUE) ? AV_NOPTS_VALUE : static_cast<int64_t>(packet.pts / DVD_TIME_BASE * AV_TIME_BASE);
  pkt.duration = (packet.duration == DVD_NOPTS_VALUE) ? 0 : static_cast<int64_t>(packet.duration / DVD_TIME_BASE * AV_TIME_BASE);
  pkt.flags = packet.bKeyFrame ? AV_PKT_FLAG_KEY : 0;

  //int ret = av_interleaved_write_frame(m_formatContext.get(), &pkt);
  int ret = av_write_frame(m_formatContext.get(), &pkt); //! @todo

  av_packet_unref(&pkt);

  if (ret < 0)
  {
    char errorDesc[AV_ERROR_MAX_STRING_SIZE] = { };
    av_strerror(ret, errorDesc, sizeof(errorDesc));
    CLog::LogF(LOGERROR, "Error while writing frame: %s", errorDesc);
    return false;
  }

  return true;
}

int64_t CMuxerFFmpeg::Seek(int64_t position, int whence)
{
  if (m_callback)
    return m_callback->SeekWrite(position, whence);

  return false;
}

void CMuxerFFmpeg::WriteOutputPacket(const uint8_t *buffer, int size)
{
  if (m_callback)
    m_callback->WriteOutputPacket(buffer, size);
}
