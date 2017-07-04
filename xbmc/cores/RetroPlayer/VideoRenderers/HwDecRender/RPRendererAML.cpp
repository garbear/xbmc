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

#include "RPRendererAML.h"

#if defined(HAS_LIBAMCODEC)
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/DVDCodecs/Video/AMLCodec.h"
#include "cores/RetroPlayer/VideoRenderers/RPVideoPicture.h"
#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "utils/ScreenshotAML.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"

CRPRendererAML::CRPRendererAML()
 : m_prevVPts(-1)
 , m_bConfigured(false)
 , m_iRenderBuffer(0)
{
}

CRPRendererAML::~CRPRendererAML()
{
}

bool CRPRendererAML::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERPRenderFormat format, void *hwPic, unsigned int orientation)
{
  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;
  m_format = format;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
  ManageRenderArea();

  m_bConfigured = true;

  for (int i = 0 ; i < m_numRenderBuffers ; ++i)
    m_buffers[i].hwDec = 0;

  return true;
}

CRPRenderInfo CRPRendererAML::GetRenderInfo()
{
  CRPRenderInfo info;
  info.formats.push_back(RP_RENDER_FMT_BYPASS);
  info.max_buffer_size = m_numRenderBuffers;
  info.optimal_buffer_size = m_numRenderBuffers;
  return info;
}

int CRPRendererAML::GetImage(YV12ImageRP *image, int source, bool readonly)
{
  if (image == nullptr)
    return -1;

  /* take next available buffer */
  if (source == -1)
   source = (m_iRenderBuffer + 1) % m_numRenderBuffers;

  return source;
}

void CRPRendererAML::AddRPVideoPictureHW(RPVideoPicture &picture, int index)
{
  BUFFER &buf = m_buffers[index];
  if (picture.hwPic)
    buf.hwDec = static_cast<CDVDAmlogicInfo*>(picture.hwPic)->Retain();
}

void CRPRendererAML::ReleaseBuffer(int idx)
{
  BUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(buf.hwDec);
    if (amli)
    {
      CAMLCodec *amlcodec;
      if (!amli->IsRendered() && (amlcodec = amli->getAmlCodec()))
        amlcodec->ReleaseFrame(amli->GetBufferIndex(), true);
      SAFE_RELEASE(amli);
    }
    buf.hwDec = NULL;
  }
}

void CRPRendererAML::FlipPage(int source)
{
  if( source >= 0 && source < m_numRenderBuffers )
    m_iRenderBuffer = source;
  else
    m_iRenderBuffer = (m_iRenderBuffer + 1) % m_numRenderBuffers;

  return;
}

bool CRPRendererAML::IsGuiLayer()
{
  return false;
}

bool CRPRendererAML::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRPRendererAML::Supports(ESCALINGMETHOD method)
{
  return false;
}

bool CRPRendererAML::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_BRIGHTNESS ||
      feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

EINTERLACEMETHOD CRPRendererAML::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

void CRPRendererAML::Reset()
{
  m_prevVPts = -1;
}

void CRPRendererAML::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  ManageRenderArea();

  CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(m_buffers[m_iRenderBuffer].hwDec);
  CAMLCodec *amlcodec = amli ? amli->getAmlCodec() : 0;

  if(amlcodec)
  {
    int pts = amli->GetOmxPts();
    if (pts != m_prevVPts)
    {
      amlcodec->ReleaseFrame(amli->GetBufferIndex());
      amlcodec->SetVideoRect(m_sourceRect, m_destRect);
      amli->SetRendered();
      m_prevVPts = pts;
    }
  }
  CAMLCodec::PollFrame();
}

#endif
