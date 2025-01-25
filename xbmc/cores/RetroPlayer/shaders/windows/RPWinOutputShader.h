/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/VideoPlayer/VideoRenderers/VideoShaders/WinVideoFilter.h"

namespace KODI
{
namespace SHADER
{

class CRPWinOutputShader : public CWinShader
{
public:
  ~CRPWinOutputShader() = default;

  bool Create(RETRO::SCALINGMETHOD scalingMethod);
  void Render(CD3DTexture& sourceTexture,
              CRect sourceRect,
              const CPoint points[4],
              CRect& viewPort,
              CD3DTexture* target,
              unsigned int range = 0);

private:
  void PrepareParameters(unsigned int sourceWidth,
                         unsigned int sourceHeight,
                         CRect sourceRect,
                         const CPoint points[4]);
  void SetShaderParameters(CD3DTexture& sourceTexture, unsigned int range, CRect& viewPort);

  unsigned int m_sourceWidth{0};
  unsigned int m_sourceHeight{0};
  CRect m_sourceRect{0.f, 0.f, 0.f, 0.f};
  CPoint m_destPoints[4] = {
      {0.f, 0.f},
      {0.f, 0.f},
      {0.f, 0.f},
      {0.f, 0.f},
  };
};

} // namespace SHADER
} // namespace KODI
