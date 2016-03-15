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
#pragma once

#include "DVDVideoCodec.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

#define PIXEL_CONVERTER_CODEC_NAME  "PixelConverter"

struct DVDVideoPicture;
struct SwsContext;

class CPixelConverter : public CDVDVideoCodec
{
public:
  CPixelConverter(CProcessInfo &processInfo);
  virtual ~CPixelConverter() { }

  // implementation of CDVDVideoCodec
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual void Dispose() override;
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts) override;
  virtual void Reset() override { }
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture) override;
  virtual void SetDropState(bool bDrop) override { }
  virtual const char* GetName() override { return PIXEL_CONVERTER_CODEC_NAME; }

private:
  AVPixelFormat    m_pixfmt;
  AVPixelFormat    m_targetfmt;
  unsigned int     m_width;
  unsigned int     m_height;
  SwsContext*      m_swsContext;
  DVDVideoPicture* m_buf;
};
