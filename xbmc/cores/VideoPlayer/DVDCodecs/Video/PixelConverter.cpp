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

#include "PixelConverter.h"
#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "utils/log.h"

extern "C" {
#include "libavutil/avconfig.h"
#include "libswscale/swscale.h"
}

#include <algorithm>
#include <cstring>

CPixelConverter::CPixelConverter(CProcessInfo &processInfo) :
  CDVDVideoCodec(processInfo),
  m_pixfmt(AV_PIX_FMT_NONE),
  m_targetfmt(AV_PIX_FMT_NONE),
  m_width(0),
  m_height(0),
  m_swsContext(nullptr),
  m_buf(nullptr)
{
}

bool CPixelConverter::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (hints.width <= 0 || hints.height <= 0)
    return false;

  m_pixfmt = hints.pixfmt;
  m_targetfmt = AV_PIX_FMT_NONE;
  m_width = hints.width;
  m_height = hints.height;

  const std::vector<ERenderFormat>& renderFmts = options.m_formats;

  // Try to render without conversion
  ERenderFormat renderFmt = CDVDCodecUtils::EFormatFromPixfmt(m_pixfmt);
  if (renderFmt != RENDER_FMT_NONE && std::find(renderFmts.begin(), renderFmts.end(), renderFmt) != renderFmts.end())
  {
    // TODO: Support non-YUV420p
    //m_targetfmt = m_pixfmt;
  }

  // Try to fallback to YUV420p
  if (m_targetfmt == AV_PIX_FMT_NONE && std::find(renderFmts.begin(), renderFmts.end(), RENDER_FMT_YUV420P) != renderFmts.end())
    m_targetfmt = AV_PIX_FMT_YUV420P;

  if (m_targetfmt == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGDEBUG, "%s: Failed to find a suitable render format", GetName());
    return false;
  }

  if (m_pixfmt != m_targetfmt)
  {
    m_swsContext = sws_getContext(m_width, m_height, m_pixfmt,
                                  m_width, m_height, m_targetfmt,
                                  SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (!m_swsContext)
    {
      CLog::Log(LOGERROR, "%s: Failed to create swscale context", GetName());
      return false;
    }
  }

  m_buf = CDVDCodecUtils::AllocatePicture(m_width, m_height);

  return true;
}

void CPixelConverter::Dispose()
{
  if (m_swsContext)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = nullptr;
  }

  if (m_buf)
  {
    CDVDCodecUtils::FreePicture(m_buf);
    m_buf = nullptr;
  }
}

int CPixelConverter::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  if (pData == nullptr || iSize == 0)
    return VC_BUFFER;

  int ret = VC_ERROR;

  if (m_swsContext)
  {
    const int stride = iSize / m_height;

    if (stride > 0)
    {
      uint8_t* src[] =       { pData,               0,                   0,                   0 };
      int      srcStride[] = { stride,              0,                   0,                   0 };
      uint8_t* dst[] =       { m_buf->data[0],      m_buf->data[1],      m_buf->data[2],      0 };
      int      dstStride[] = { m_buf->iLineSize[0], m_buf->iLineSize[1], m_buf->iLineSize[2], 0 };

      sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

      ret = VC_PICTURE;
    }
  }
  else
  {
    // Don't overflow buffer
    if (iSize <= (int)m_width * (int)m_height * 3 / 2)
    {
      std::memcpy(m_buf->data[0], pData, iSize);
      ret = VC_PICTURE;
    }
  }

  return ret;
}

bool CPixelConverter::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  pDvdVideoPicture->dts            = DVD_NOPTS_VALUE;
  pDvdVideoPicture->pts            = DVD_NOPTS_VALUE;

  for (int i = 0; i < 4; i++)
  {
    pDvdVideoPicture->data[i]      = m_buf->data[i];
    pDvdVideoPicture->iLineSize[i] = m_buf->iLineSize[i];
  }

  pDvdVideoPicture->iFlags         = 0; // *not* DVP_FLAG_ALLOCATED
  pDvdVideoPicture->color_matrix   = 4; // CONF_FLAGS_YUVCOEF_BT601
  pDvdVideoPicture->color_range    = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
  pDvdVideoPicture->iWidth         = m_width;
  pDvdVideoPicture->iHeight        = m_height;
  pDvdVideoPicture->iDisplayWidth  = m_width;
  pDvdVideoPicture->iDisplayHeight = m_height;
  pDvdVideoPicture->format         = CDVDCodecUtils::EFormatFromPixfmt(m_targetfmt);

  return true;
}
