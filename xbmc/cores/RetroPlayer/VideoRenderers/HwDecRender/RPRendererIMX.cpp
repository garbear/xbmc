/*
 *      Copyright (C) 2007-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPRendererIMX.h"

#if defined(HAS_IMXVPU)
#include "cores/IPlayer.h"
#include "windowing/egl/EGLWrapper.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "cores/RetroPlayer/VideoRenderers/RPRenderFlags.h"
#include "cores/RetroPlayer/VideoRenderers/RPVideoPicture.h"

#define RENDER_FLAG_FIELDS (RENDER_FLAG_FIELD0 | RENDER_FLAG_FIELD1)

CRPRendererIMX::CRPRendererIMX()
{
  m_bufHistory.clear();
}

CRPRendererIMX::~CRPRendererIMX()
{
  UnInit();
  std::for_each(m_bufHistory.begin(), m_bufHistory.end(), Release);
  g_IMX.Deinitialize();
}

void CRPRendererIMX::AddRPVideoPictureHW(DVDRPVideoPicture &picture, int index)
{
  YUVBufferRP &buf = m_buffers[index];

  buf.hwDec = picture.IMXBuffer;

  if (picture.IMXBuffer)
    picture.IMXBuffer->Lock();
}

void CRPRendererIMX::ReleaseBuffer(int idx)
{
  CDVDVideoCodecIMXBuffer *buffer =  static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[idx].hwDec);
  SAFE_RELEASE(buffer);
  m_buffers[idx].hwDec = NULL;
}

int CRPRendererIMX::GetImageHook(YV12ImageRP *image, int source, bool readonly)
{
  return source;
}

bool CRPRendererIMX::IsGuiLayer()
{
  return false;
}

bool CRPRendererIMX::Supports(ERENDERFEATURE feature)
{
  if (!g_IMXContext.IsZoomAllowed())
    return false;

  if (feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_ZOOM)
    return true;

  return false;
}

bool CRPRendererIMX::Supports(ESCALINGMETHOD method)
{
  return method == VS_SCALINGMETHOD_AUTO;
}

bool CRPRendererIMX::WantsDoublePass()
{
  if (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod ==
      VS_INTERLACEMETHOD_IMX_ADVMOTION)
    return true;
  else
    return false;
}

CRPRenderInfo CRPRendererIMX::GetRenderInfo()
{
  CRPRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  // Let the codec control the buffer size
  info.optimal_buffer_size = info.max_buffer_size;
  return info;
}

bool CRPRendererIMX::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using IMXMAP render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_IMXMAP;
  return true;
}

bool CRPRendererIMX::RenderHook(int index)
{
  return true;// nothing to be done for imx
}

bool CRPRendererIMX::RenderUpdateVideoHook(bool clear, DWORD flags, DWORD alpha)
{
  static DWORD flagsPrev;
#if 0
  static unsigned long long previous = 0;
  unsigned long long current = XbmcThreads::SystemClockMillis();
  printf("r->r: %d\n", (int)(current-previous));
  previous = current;
#endif
  CDVDVideoCodecIMXBuffer *buffer = static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[m_iYV12RenderBuffer].hwDec);
  if (buffer)
  {
    if (!m_bufHistory.empty() && m_bufHistory.back() != buffer || m_bufHistory.empty())
    {
      buffer->Lock();
      m_bufHistory.push_back(buffer);
    }
    else if (!m_bufHistory.empty() && m_bufHistory.back() == buffer && flagsPrev == flags)
    {
      g_IMX.WaitVsync();
      return true;
    }

    flagsPrev = flags;

    int size = flags & RENDER_FLAG_FIELDMASK ? 2 : 1;
    while (m_bufHistory.size() > size)
    {
      m_bufHistory.front()->Release();
      m_bufHistory.pop_front();
    }

    // this hack is needed to get the 2D mode of a 3D movie going
    RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
    if (stereo_mode)
      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);

    ManageRenderArea();

    if (stereo_mode)
      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_OFF);

    CRect dstRect(m_destRect);
    CRect srcRect(m_sourceRect);
    switch (stereo_mode)
    {
      case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
        dstRect.y2 *= 2.0;
        srcRect.y2 *= 2.0;
      break;

      case RENDER_STEREO_MODE_SPLIT_VERTICAL:
        dstRect.x2 *= 2.0;
        srcRect.x2 *= 2.0;
      break;

      default:
      break;
    }

    //CLog::Log(LOGDEBUG, "BLIT RECTS: source x1 %f x2 %f y1 %f y2 %f dest x1 %f x2 %f y1 %f y2 %f", srcRect.x1, srcRect.x2, srcRect.y1, srcRect.y2, dstRect.x1, dstRect.x2, dstRect.y1, dstRect.y2);
    uint8_t fieldFmt = flags & RENDER_FLAG_FIELDMASK;

    if (!g_graphicsContext.IsFullScreenVideo())
      flags &= ~RENDER_FLAG_FIELDS;

    if (flags & RENDER_FLAG_FIELDS)
    {
      fieldFmt |= IPU_DEINTERLACE_RATE_EN;
      if (flags & RENDER_FLAG_FIELD1)
      {
        fieldFmt |= IPU_DEINTERLACE_RATE_FRAME1;
        // CXBMCRPRenderManager::PresentFields() is swapping field flag for frame1
        // this makes IPU render same picture as before, just shifted one line.
        // let's correct this
        fieldFmt ^= RENDER_FLAG_FIELDMASK;
      }
    }

    CDVDVideoCodecIMXBuffer *buffer_p = m_bufHistory.front();
    g_IMXContext.Blit(buffer_p == buffer ? nullptr : buffer_p, buffer, srcRect, dstRect, fieldFmt);
  }

#if 0
  unsigned long long current2 = XbmcThreads::SystemClockMillis();
  printf("r: %d  %d\n", m_iYV12RenderBuffer, (int)(current2-current));
#endif

  g_IMXContext.WaitVSync();
  return true;
}

bool CRPRendererIMX::CreateTexture(int index)
{
  return true;
}

void CRPRendererIMX::DeleteTexture(int index)
{
  ReleaseBuffer(index);
}

bool CRPRendererIMX::UploadTexture(int index)
{
  return true;// nothing todo for IMX
}
#endif
