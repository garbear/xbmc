#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPRenderFormats.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

struct RPVideoPicture;
struct YV12ImageRP;

namespace RPRenderUtils {

struct ERPFormatMap {
  AVPixelFormat   pix_fmt;
  ERPRenderFormat format;
};

static const ERPFormatMap g_format_map[] = {
   { AV_PIX_FMT_YUV420P,     RP_RENDER_FMT_YUV420P }
  ,{ AV_PIX_FMT_YUVJ420P,    RP_RENDER_FMT_YUV420P }
  ,{ AV_PIX_FMT_YUV420P10,   RP_RENDER_FMT_YUV420P10 }
  ,{ AV_PIX_FMT_YUV420P16,   RP_RENDER_FMT_YUV420P16 }
  ,{ AV_PIX_FMT_UYVY422,     RP_RENDER_FMT_UYVY422 }
  ,{ AV_PIX_FMT_YUYV422,     RP_RENDER_FMT_YUYV422 }
  ,{ AV_PIX_FMT_VAAPI_VLD,   RP_RENDER_FMT_VAAPI }
  ,{ AV_PIX_FMT_D3D11VA_VLD, RP_RENDER_FMT_DXVA }
  ,{ AV_PIX_FMT_NONE     ,   RP_RENDER_FMT_NONE }
};

RPVideoPicture* AllocatePicture(int iWidth, int iHeight);
void FreePicture(RPVideoPicture* pPicture);
bool CopyPicture(RPVideoPicture* pDst, RPVideoPicture* pSrc);
bool CopyPicture(YV12ImageRP* pImage, RPVideoPicture *pSrc);
bool CopyNV12Picture(YV12ImageRP* pImage, RPVideoPicture *pSrc);
bool CopyYUV422PackedPicture(YV12ImageRP* pImage, RPVideoPicture *pSrc);

ERPRenderFormat EFormatFromPixfmt(int fmt);
AVPixelFormat PixfmtFromEFormat(ERPRenderFormat fmt);

} // namespace RPRenderUtils
