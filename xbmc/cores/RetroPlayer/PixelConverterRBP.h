/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "PixelConverter.h"

#include <stdint.h>
#include <vector>

class CMMALPool;
struct RPVideoPicture;

class CPixelConverterRBP : public CPixelConverter
{
public:
  CPixelConverterRBP();
  ~CPixelConverterRBP() override { Dispose(); }

  // implementation of IPixelConverter
  bool Open(AVPixelFormat pixfmt, AVPixelFormat target, unsigned int width, unsigned int height) override;
  void Dispose() override;
  bool Decode(const uint8_t* pData, unsigned int size) override;
  void GetPicture(RPVideoPicture& dvdRPVideoPicture) override;

private:
  /*!
   * \brief Allocate a new picture (AV_PIX_FMT_YUV420P)
   */
  RPVideoPicture* AllocatePicture(int iWidth, int iHeight);

  /*!
   * \brief Free an allocated picture
   */
  void FreePicture(RPVideoPicture* pPicture);

  struct PixelFormatTargetTable
  {
    AVPixelFormat pixfmt;
    AVPixelFormat targetfmt;
  };

  struct MMALEncodingTable
  {
    AVPixelFormat pixfmt;
    uint32_t      encoding;
  };

  static std::vector<PixelFormatTargetTable> pixfmt_target_table;
  static std::vector<MMALEncodingTable> mmal_encoding_table;

  static AVPixelFormat TranslateTargetFormat(AVPixelFormat pixfmt);
  static uint32_t TranslateFormat(AVPixelFormat pixfmt);

  std::shared_ptr<CMMALPool> m_pool;
  uint32_t m_mmal_format;
};
