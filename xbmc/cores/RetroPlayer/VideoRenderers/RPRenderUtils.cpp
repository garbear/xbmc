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
 *   GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPRenderUtils.h"
#include "RPVideoPicture.h"
#include <xbmc/utils/log.h>
#include "RPRenderManager.h"

extern "C" {
#include <libavformat/avio.h>
}

RPVideoPicture* RPRenderUtils::AllocatePicture(int iWidth, int iHeight)
{
  RPVideoPicture* pPicture = new RPVideoPicture;
  if (pPicture)
  {
    pPicture->iWidth = iWidth;
    pPicture->iHeight = iHeight;

    int w = (iWidth + 1) / 2;
    int h = (iHeight + 1) / 2;
    int size = w * h;
    int totalsize = (iWidth * iHeight) + size * 2;
    uint8_t* data = static_cast<uint8_t*>(av_malloc(totalsize));
    if (data)
    {
      pPicture->data[0] = data;
      pPicture->data[1] = pPicture->data[0] + (iWidth * iHeight);
      pPicture->data[2] = pPicture->data[1] + size;
      pPicture->data[3] = nullptr;
      pPicture->iLineSize[0] = iWidth;
      pPicture->iLineSize[1] = w;
      pPicture->iLineSize[2] = w;
      pPicture->iLineSize[3] = 0;
    }
    else
    {
      CLog::Log(LOGFATAL, "RPRenderUtils::AllocatePicture, unable to allocate new video picture, out of memory.");
      delete pPicture;
      pPicture = nullptr;
    }
  }
  return pPicture;
}

void RPRenderUtils::FreePicture(RPVideoPicture* pPicture)
{
  av_free(pPicture->data[0]);
  delete pPicture;
}

bool RPRenderUtils::CopyPicture(RPVideoPicture* pDst, RPVideoPicture* pSrc)
{
  uint8_t *s, *d;
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;

  s = pSrc->data[0];
  d = pDst->data[0];

  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[0];
    d += pDst->iLineSize[0];
  }

  w >>= 1;
  h >>= 1;

  s = pSrc->data[1];
  d = pDst->data[1];
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[1];
    d += pDst->iLineSize[1];
  }

  s = pSrc->data[2];
  d = pDst->data[2];
  for (int y = 0; y < h; y++)
  {
    memcpy(d, s, w);
    s += pSrc->iLineSize[2];
    d += pDst->iLineSize[2];
  }
  return true;
}

bool RPRenderUtils::CopyPicture(YV12ImageRP* pImage, RPVideoPicture *pSrc)
{
  uint8_t *s = pSrc->data[0];
  uint8_t *d = pImage->plane[0];
  int w = pImage->width * pImage->bpp;
  int h = pImage->height;
  if ((w == pSrc->iLineSize[0]) && (static_cast<unsigned int>(pSrc->iLineSize[0]) == pImage->stride[0]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[0];
      d += pImage->stride[0];
    }
  }
  s = pSrc->data[1];
  d = pImage->plane[1];
  w = (pImage->width >> pImage->cshift_x) * pImage->bpp;
  h = (pImage->height >> pImage->cshift_y);
  if ((w == pSrc->iLineSize[1]) && (static_cast<unsigned int>(pSrc->iLineSize[1]) == pImage->stride[1]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[1];
      d += pImage->stride[1];
    }
  }
  s = pSrc->data[2];
  d = pImage->plane[2];
  if ((w == pSrc->iLineSize[2]) && (static_cast<unsigned int>(pSrc->iLineSize[2]) == pImage->stride[2]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[2];
      d += pImage->stride[2];
    }
  }
  return true;
}

bool RPRenderUtils::CopyNV12Picture(YV12ImageRP* pImage, RPVideoPicture *pSrc)
{
  uint8_t *s = pSrc->data[0];
  uint8_t *d = pImage->plane[0];
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;
  // Copy Y
  if ((w == pSrc->iLineSize[0]) && ((unsigned int)pSrc->iLineSize[0] == pImage->stride[0]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[0];
      d += pImage->stride[0];
    }
  }

  s = pSrc->data[1];
  d = pImage->plane[1];
  w = pSrc->iWidth;
  h = pSrc->iHeight >> 1;
  // Copy packed UV (width is same as for Y as it's both U and V components)
  if ((w == pSrc->iLineSize[1]) && (static_cast<unsigned int>(pSrc->iLineSize[1]) == pImage->stride[1]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->iLineSize[1];
      d += pImage->stride[1];
    }
  }

  return true;
}

bool RPRenderUtils::CopyYUV422PackedPicture(YV12ImageRP* pImage, RPVideoPicture* pSrc)
{
  uint8_t *s = pSrc->data[0];
  uint8_t *d = pImage->plane[0];
  int w = pSrc->iWidth;
  int h = pSrc->iHeight;

  // Copy YUYV
  if ((w * 2 == pSrc->iLineSize[0]) && (static_cast<unsigned int>(pSrc->iLineSize[0]) == pImage->stride[0]))
  {
    memcpy(d, s, w*h * 2);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w * 2);
      s += pSrc->iLineSize[0];
      d += pImage->stride[0];
    }
  }

  return true;
}

ERPRenderFormat RPRenderUtils::EFormatFromPixfmt(int fmt)
{
  for (const ERPFormatMap *p = g_format_map; p->pix_fmt != AV_PIX_FMT_NONE; ++p)
  {
    if (p->pix_fmt == fmt)
      return p->format;
  }
  return RP_RENDER_FMT_NONE;
}

AVPixelFormat RPRenderUtils::PixfmtFromEFormat(ERPRenderFormat fmt)
{
  for (const ERPFormatMap *p = g_format_map; p->pix_fmt != AV_PIX_FMT_NONE; ++p)
  {
    if (p->format == fmt)
      return p->pix_fmt;
  }
  return AV_PIX_FMT_NONE;
}
