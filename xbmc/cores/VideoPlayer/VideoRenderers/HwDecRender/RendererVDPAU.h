/*
 *      Copyright (C) 2007-2015 Team XBMC
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

#pragma once

#include "system.h"

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

class CRendererVDPAU : public CLinuxRendererGL
{
public:
  CRendererVDPAU();
  ~CRendererVDPAU() override;

  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height,
                         float fps, unsigned flags, ERenderFormat format, void *hwPic, unsigned int orientation) override;

  // Player functions
  void AddVideoPictureHW(VideoPicture &picture, int index) override;
  void ReleaseBuffer(int idx) override;
  CRenderInfo GetRenderInfo() override;
  bool ConfigChanged(void *hwPic) override;

  // Feature support
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

protected:
  bool LoadShadersHook() override;
  bool RenderHook(int idx) override;

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  bool CreateVDPAUTexture(int index);
  void DeleteVDPAUTexture(int index);
  bool UploadVDPAUTexture(int index);

  bool CreateVDPAUTexture420(int index);
  void DeleteVDPAUTexture420(int index);
  bool UploadVDPAUTexture420(int index);

  EShaderFormat GetShaderFormat(ERenderFormat renderFormat) override;

  bool m_isYuv = false;
};


