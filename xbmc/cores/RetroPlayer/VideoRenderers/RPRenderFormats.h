#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
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

#include <cstddef>
#include <vector>
#include "cores/IPlayer.h"

enum ERPRenderFormat {
  RP_RENDER_FMT_NONE = 0,
  RP_RENDER_FMT_YUV420P,
  RP_RENDER_FMT_YUV420P10,
  RP_RENDER_FMT_YUV420P16,
  RP_RENDER_FMT_VDPAU,
  RP_RENDER_FMT_NV12,
  RP_RENDER_FMT_UYVY422,
  RP_RENDER_FMT_YUYV422,
  RP_RENDER_FMT_DXVA,
  RP_RENDER_FMT_VAAPI,
  RP_RENDER_FMT_CVBREF,
  RP_RENDER_FMT_BYPASS,
  RP_RENDER_FMT_MEDIACODEC,
  RP_RENDER_FMT_MEDIACODECSURFACE,
  RP_RENDER_FMT_IMXMAP,
  RP_RENDER_FMT_MMAL,
  RP_RENDER_FMT_AML,
};

struct CRPRenderInfo
{
  CRPRenderInfo()
  {
    Reset();
  }
  void Reset()
  {
    optimal_buffer_size = 0;
    max_buffer_size = 0;
    opaque_pointer = nullptr;
    formats.clear();
  }
  unsigned int optimal_buffer_size;
  unsigned int max_buffer_size;
  // Supported pixel formats, can be called before configure
  std::vector<ERPRenderFormat> formats;
  // Can be used for initialising video codec with information from renderer (e.g. a shared image pool)
  void *opaque_pointer;
};
