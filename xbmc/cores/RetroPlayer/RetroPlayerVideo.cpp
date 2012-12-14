/*
 *      Copyright (C) 2012 Garrett Brown
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "RetroPlayerVideo.h"
#include "cores/dvdplayer/DVDClock.h" // for DVD_NOPTS_VALUE
#include "cores/dvdplayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "utils/log.h"

#define CLAMP(a, min, max) ((a) > (max) ? (max) : ((a) < (min) ? (min) : (a)))

CRetroPlayerVideo::CRetroPlayerVideo()
  : CThread("RetroPlayerVideo"),
    m_pixelFormat(RETRO_PIXEL_FORMAT_0RGB1555),
    m_swsContext(NULL),
    m_bAllowFullscreen(false),
    m_framerate(0.0),
    m_outputWidth(0),
    m_outputHeight(0),
    m_outputFramerate(0.0),
    m_bPaused(false)
{
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  StopThread();
}

void CRetroPlayerVideo::GoForth(double framerate)
{
  m_framerate = framerate;
  Create();
}

void CRetroPlayerVideo::Process()
{
  Frame frame;
  DVDVideoPicture *pPicture;

  m_dllSwScale.Load();

  while (!m_bStop)
  {
    if (m_bPaused)
    {
      m_pauseEvent.Wait();
      continue;
    }

    { // Enter critical section
      CSingleLock lock(m_critSection);
      if (!m_frames.size())
      {
        lock.Leave();
        // Assume a frame is late if the jitter is double the frame's display time
        if (!m_frameReady.WaitMSec((int)(1000 * 2 / m_framerate)))
          m_frameReady.Reset();
        else
          CLog::Log(LOGNOTICE, "RetroPlayerVideo: Timeout waiting for frame, status is %s",
              !m_bStop ? "running" : "not running");
        // If event wasn't triggered, we might be done, so check m_bStop again
        continue;
      }
      // If several frames built up, ignore all but the most recent
      while (m_frames.size())
      {
        // Clean up any ignored frames
        delete[] (uint8_t*)frame.data;
        frame = m_frames.front();
        m_frames.pop();
      }
    } // End critical section

    pPicture = CDVDCodecUtils::AllocatePicture(frame.width, frame.height);
    if (!pPicture)
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to allocate picture");
      break;
    }

    pPicture->dts = DVD_NOPTS_VALUE;
    pPicture->pts = DVD_NOPTS_VALUE;
    pPicture->format = RENDER_FMT_YUV420P; // PIX_FMT_YUV420P
    pPicture->color_range  = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
    pPicture->color_matrix = 4; // CONF_FLAGS_YUVCOEF_BT601
    pPicture->iFlags  = DVP_FLAG_ALLOCATED;
    pPicture->iDisplayWidth  = frame.width;
    pPicture->iDisplayHeight = frame.height;
    pPicture->iDuration = 1 / m_framerate;

    // Got the picture, now make sure we're ready to render it
    if (!CheckConfiguration(*pPicture))
      break;

    // CheckConfiguration() should have set up our SWScale context
    if (!m_swsContext)
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to grab SWScale context");
      break;
    }

    // Colorspace conversion
    uint8_t *src[] = { reinterpret_cast<uint8_t*>(const_cast<void*>(frame.data)), 0, 0, 0 };
    int      srcStride[] = { frame.pitch, 0, 0, 0 };
    uint8_t *dst[] = { pPicture->data[0], pPicture->data[1], pPicture->data[2], 0 };
    int      dstStride[] = { pPicture->iLineSize[0], pPicture->iLineSize[1], pPicture->iLineSize[2], 0 };

    m_dllSwScale.sws_scale(m_swsContext, src, srcStride, 0, frame.height, dst, dstStride);

    // Get ready to drop the picture off on RenderManger's doorstep
    if (!g_renderManager.IsStarted())
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Renderer not started");
      break;
    }

    int index = g_renderManager.AddVideoPicture(*pPicture);
    g_renderManager.FlipPage(CThread::m_bStop);
    CDVDCodecUtils::FreePicture(pPicture);

    // Clean up the data allocated in CRetroPlayer::OnVideoFrame()
    delete[] (uint8_t*)frame.data;
    frame.data = NULL;
  }

  if (m_swsContext)
    m_dllSwScale.sws_freeContext(m_swsContext);

  m_dllSwScale.Unload();

  m_bStop = true;
}



bool CRetroPlayerVideo::CheckConfiguration(const DVDVideoPicture &picture)
{
  double framerate = 1 / picture.iDuration;

  if (!g_renderManager.IsConfigured() ||
      m_outputWidth != picture.iWidth ||
      m_outputHeight != picture.iHeight ||
      m_outputFramerate != framerate)
  {
    unsigned int flags = 0;
    flags |= CONF_FLAGS_YUVCOEF_BT601; // picture.color_matrix = 4
    if (picture.color_range == 1)
      flags |= CONF_FLAGS_YUV_FULLRANGE;
    if (m_bAllowFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
      m_bAllowFullscreen = false; // only allow on first configure
    }

    CLog::Log(LOGDEBUG, "RetroPlayerVideo: Change configuration: %dx%d, %4.2f fps", picture.iWidth, picture.iHeight, framerate);
    int orientation = 0; // (90 = 5, 180 = 2, 270 = 7), if we ever want to use RETRO_ENVIRONMENT_SET_ROTATION
    if (!g_renderManager.Configure(picture.iWidth, picture.iHeight, picture.iDisplayWidth, picture.iDisplayHeight, (float)framerate, flags, picture.format, picture.extended_format, orientation))
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to configure renderer");
      return false;
    }

    m_outputWidth = picture.iWidth;
    m_outputHeight = picture.iHeight;
    m_outputFramerate = framerate;

    PixelFormat format;
    switch (m_pixelFormat)
    {
    case RETRO_PIXEL_FORMAT_XRGB8888:
      CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: XRGB8888");
      format = PIX_FMT_ARGB;
      break;
    case RETRO_PIXEL_FORMAT_RGB565:    
  	  CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: RGB565");
      format = PIX_FMT_RGB565LE;
      break;
    case RETRO_PIXEL_FORMAT_0RGB1555:
    default:
      CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: RGB1555");
      format = PIX_FMT_RGB555LE;
      break;
    }

#ifdef __BIG_ENDIAN__
    if (format == PIX_FMT_RGB555LE)
      format = PIX_FMT_RGB555BE;
#endif

    if (m_swsContext)
      m_dllSwScale.sws_freeContext(m_swsContext);

    m_swsContext = m_dllSwScale.sws_getContext(
      picture.iWidth,    picture.iHeight,    format,
      picture.iWidth, picture.iHeight, PIX_FMT_YUV420P,
      SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL
    );
  }
  return true;
}

void CRetroPlayerVideo::SendVideoFrame(const void *data, unsigned width, unsigned height, size_t pitch)
{
  if (IsRunning())
  {
    CSingleLock lock(m_critSection);
    m_frames.push(Frame(data, width, height, pitch));
  }
  else
  {
    // Frame was discarded, so make sure we clean up after CRetroPlayer::OnVideoFrame()
    delete[] (uint8_t*)data;
  }
}
