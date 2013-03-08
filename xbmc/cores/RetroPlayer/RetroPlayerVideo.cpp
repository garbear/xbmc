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

CRetroPlayerVideo::CRetroPlayerVideo()
  : CThread("RetroPlayerVideo"),
    m_queuedFrame(),
    m_pixelFormat(RETRO_PIXEL_FORMAT_0RGB1555),
    m_swsContext(NULL),
    m_bAllowFullscreen(false),
    m_framerate(0.0),
    m_outputWidth(0),
    m_outputHeight(0),
    m_outputFramerate(0.0)
{
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  StopThread();
}

void CRetroPlayerVideo::GoForth(double framerate, bool fullscreen)
{
  m_framerate = framerate;
  m_bAllowFullscreen = fullscreen;
  Create();
}

void CRetroPlayerVideo::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  m_frameEvent.Set();
  CThread::StopThread(bWait);
}

void CRetroPlayerVideo::Process()
{
  static Frame frame = {}; // static for persistence
  DVDVideoPicture *pPicture = NULL;

  if (!m_dllSwScale.Load())
    return;

  while (!m_bStop)
  {
    // 1s should be a good failsafe if the event isn't triggered (shouldn't happen)
    m_frameEvent.WaitMSec(1000);
    if (m_bStop)
      break;

    {
      CSingleLock lock(m_critSection);

      // Only proceed if we have a frame to render
	  if (m_queuedFrame.data.empty())
        continue;

      // Deep copy
      frame = m_queuedFrame;
	  // std::vector's copy constructor already copied the data by value, no memcpy required
    }

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
      CLog::Log(LOGERROR, "RetroPlayerVideo: Failed to grab SWScale context, bailing");
      break;
    }

    // Colorspace conversion
	uint8_t *src[] = { frame.data.data(), 0, 0, 0 };
    int      srcStride[] = { frame.pitch, 0, 0, 0 };
    uint8_t *dst[] = { pPicture->data[0], pPicture->data[1], pPicture->data[2], 0 };
    int      dstStride[] = { pPicture->iLineSize[0], pPicture->iLineSize[1], pPicture->iLineSize[2], 0 };

    m_dllSwScale.sws_scale(m_swsContext, src, srcStride, 0, frame.height, dst, dstStride);

    // Clean up the data allocated in CRetroPlayer::OnVideoFrame()
    frame.data.clear(); // static - don't delete memory, just contents

    // Get ready to drop the picture off on RenderManger's doorstep
    if (!g_renderManager.IsStarted())
    {
      CLog::Log(LOGERROR, "RetroPlayerVideo: Renderer not started, bailing");
      break;
    }

    int index = g_renderManager.AddVideoPicture(*pPicture);
    (void)index; // disable warning
    g_renderManager.FlipPage(CThread::m_bStop);
    CDVDCodecUtils::FreePicture(pPicture);
    pPicture = NULL;
  }

  {
    CSingleLock lock(m_critSection);
	m_queuedFrame.data.clear();
    // Don't want to allocate more data in SendVideoFrame()
    m_bStop = true;
  }

  // In case we hit a break above
  frame.data.clear();
  if (pPicture)
    CDVDCodecUtils::FreePicture(pPicture);

  if (m_swsContext)
    m_dllSwScale.sws_freeContext(m_swsContext);

  m_dllSwScale.Unload();
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
      CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: XRGB8888, using PIX_FMT_0RGB32");
      format = PIX_FMT_0RGB32;
      break;
    case RETRO_PIXEL_FORMAT_RGB565:
  	  CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: RGB565, using PIX_FMT_RGB565");
      format = PIX_FMT_RGB565;
      break;
    case RETRO_PIXEL_FORMAT_0RGB1555:
    default:
      CLog::Log(LOGINFO, "RetroPlayerVideo: Pixel Format: 0RGB1555, using PIX_FMT_RGB555");
      format = PIX_FMT_RGB555;
      break;
    }

    if (m_swsContext)
      m_dllSwScale.sws_freeContext(m_swsContext);

    // Colorspace conversion
    m_swsContext = m_dllSwScale.sws_getContext(
      picture.iWidth, picture.iHeight, format,
      picture.iWidth, picture.iHeight, PIX_FMT_YUV420P,
      SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL
    );
  }
  return true;
}

void CRetroPlayerVideo::SendVideoFrame(const void *data, unsigned width, unsigned height, size_t pitch)
{
  CSingleLock lock(m_critSection);

  if (!m_bStop && IsRunning())
  {
    // No need for delete[] and new[], just resize
	m_queuedFrame.data.resize(pitch * height);

    memcpy(m_queuedFrame.data.data(), data, pitch * height);
    m_queuedFrame.width  = width;
    m_queuedFrame.height = height;
    m_queuedFrame.pitch  = pitch;

    // Notify the video thread that we're ready to display the picture. If
    // m_frameEvent is signaled before hitting Wait() above, then it will have
    // to wait until the next SendVideoFrame() before continuing.
    m_frameEvent.Set();
  }
}
