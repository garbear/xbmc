/*
 *  Copyright (C) 2005-2013 Team XBMC
 *  http://xbmc.org
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

#include "OverlayRenderer.h"
#include <string>

enum SubtitleAlign
{
  SUBTITLE_ALIGN_MANUAL         = 0,
  SUBTITLE_ALIGN_BOTTOM_INSIDE,
  SUBTITLE_ALIGN_BOTTOM_OUTSIDE,
  SUBTITLE_ALIGN_TOP_INSIDE,
  SUBTITLE_ALIGN_TOP_OUTSIDE
};

class CGUITextLayout;
class CDVDOverlayText;

namespace OVERLAY {

class COverlayText : public COverlay
{
public:
  COverlayText() {}
  COverlayText(CDVDOverlayText* src);
  ~COverlayText() override;
  void Render(SRenderState& state) override;
  using COverlay::PrepareRender;
  void PrepareRender(const std::string &font, int color, int height, int style,
                     const std::string &fontcache, const std::string &fontbordercache);
  virtual CGUITextLayout* GetFontLayout(const std::string &font, int color, int height, int style,
                                        const std::string &fontcache, const std::string &fontbordercache);

  CGUITextLayout* m_layout;
  std::string m_text;
  int m_subalign;
};

}
