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

#include "RetroPlayerVideo.h"
#include "PixelConverter.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "utils/log.h"

#include <atomic> // TODO

using namespace GAME;

CRetroPlayerVideo::CRetroPlayerVideo(CDVDClock& clock, CRenderManager& renderManager, CProcessInfo& processInfo) :
  //CThread("RetroPlayerVideo"),
  m_clock(clock),
  m_renderManager(renderManager),
  m_processInfo(processInfo),
  m_framerate(0.0),
  m_bConfigured(false),
  m_droppedFrames(0)
{
  m_renderManager.PreInit();
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  CloseStream();
  m_renderManager.UnInit();
}

bool CRetroPlayerVideo::OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, double framerate)
{
  CLog::Log(LOGINFO, "RetroPlayerVideo: Creating video stream with pixel format: %i, %dx%d", pixfmt, width, height);

  m_framerate = framerate;
  m_bConfigured = false;
  m_droppedFrames = 0;
  m_pixelConverter.reset(new CPixelConverter);
  if (m_pixelConverter->Open(pixfmt, AV_PIX_FMT_YUV420P, width, height))
    return true;

  m_pixelConverter.reset();

  return false;
}

bool CRetroPlayerVideo::OpenEncodedStream(AVCodecID codec)
{
  /* TODO
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
  */
  return false;
}

void CRetroPlayerVideo::AddData(const uint8_t* data, unsigned int size)
{
  DVDVideoPicture picture = { };

  if (GetPicture(data, size, picture))
  {
    if (!Configure(picture))
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to configure renderer");
      CloseStream();
    }
    else
    {
      SendPicture(picture);
    }
  }
}

void CRetroPlayerVideo::CloseStream()
{
  m_pixelConverter.reset();
}

bool CRetroPlayerVideo::Configure(DVDVideoPicture& picture)
{
  if (!m_bConfigured)
  {
    // Determine RenderManager flags
    unsigned int flags = CONF_FLAGS_YUVCOEF_BT601 | // color_matrix = 4
                         CONF_FLAGS_FULLSCREEN;     // Allow fullscreen

    const int orientation = 0; // (90 = 5, 180 = 2, 270 = 7) if we ever want to use RETRO_ENVIRONMENT_SET_ROTATION

    const int buffers = 1; // TODO

    m_bConfigured = m_renderManager.Configure(picture, static_cast<float>(m_framerate), flags, orientation, buffers);
  }

  return m_bConfigured;
}

bool CRetroPlayerVideo::GetPicture(const uint8_t* data, unsigned int size, DVDVideoPicture& picture)
{
  bool bHasPicture = false;

  int lateframes;
  double renderPts;
  int queued, discard;
  m_renderManager.GetStats(lateframes, renderPts, queued, discard);

  if (queued == 0)
  {
    if (m_pixelConverter && m_pixelConverter->Decode(data, size))
    {
      m_pixelConverter->GetPicture(picture);
      bHasPicture = true;
    }
  }

  return bHasPicture;
}

void CRetroPlayerVideo::SendPicture(DVDVideoPicture& picture)
{
  std::atomic_bool bAbortOutput(false); // TODO

  int index = m_renderManager.AddVideoPicture(picture);
  if (index < 0)
  {
    // Video device might not be done yet, drop the frame
    m_droppedFrames++;
  }
  else
  {
    m_renderManager.FlipPage(bAbortOutput);
  }
}
