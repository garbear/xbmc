/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Texture.h"
#include "TextureGLES.h"

#include "system_gl.h"

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
class CGLTexture : public CTexture
{
public:
  CGLTexture(unsigned int width = 0,
             unsigned int height = 0,
             XB_FMT format = XB_FMT_A8R8G8B8,
             GLuint texture = 0);
  ~CGLTexture() override;

  void CreateTextureObject() override;
  void DestroyTextureObject() override;
  void LoadToGPU() override;
  void SyncGPU() override;
  void BindToUnit(unsigned int unit) override;

  bool SupportsFormat(KD_TEX_FMT textureFormat, KD_TEX_SWIZ textureSwizzle) override
  {
    return true;
  }

  GLuint getMTexture() const;

protected:
  void SetSwizzle();
  TextureFormat GetFormatGL(KD_TEX_FMT textureFormat);

  GLuint m_texture{0};
  bool m_isOglVersion3orNewer{false};
  bool m_isOglVersion33orNewer{false};
};
