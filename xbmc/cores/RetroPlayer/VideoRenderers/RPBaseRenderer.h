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

#include <utility>
#include <vector>

#include "guilib/Resolution.h"
#include "guilib/Geometry.h"
#include "RPRenderFormats.h"
#include "VideoShaders/ShaderFormats.h"
#include "cores/IPlayer.h"
#include "RPRenderFormats.h"

#define MAX_PLANES 3
#define MAX_FIELDS 3
#define NUM_BUFFERS 6

class CSetting;

typedef struct YV12ImageRP
{
  uint8_t* plane[MAX_PLANES];
  int      planesize[MAX_PLANES];
  unsigned stride[MAX_PLANES];
  unsigned width;
  unsigned height;
  unsigned flags;

  unsigned cshift_x; /* this is the chroma shift used */
  unsigned cshift_y;
  unsigned bpp; /* bytes per pixel */
} YV12ImageRP;

enum ERPFIELDSYNC
{
  RP_FS_NONE,
  RP_FS_TOP,
  RP_FS_BOT
};

// Render Methods
enum RPRenderMethods
{
  RP_RENDER_METHOD_AUTO     = 0,
  RP_RENDER_METHOD_ARB,
  RP_RENDER_METHOD_GLSL,
  RP_RENDER_METHOD_SOFTWARE,
  RP_RENDER_METHOD_D3D_PS,
  RP_RENDER_METHOD_DXVA
};

struct RPVideoPicture;

class CRPBaseRenderer
{
public:
  CRPBaseRenderer();
  virtual ~CRPBaseRenderer();

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERPRenderFormat format, void* hwPic, unsigned int orientation) = 0;
  virtual bool IsConfigured() = 0;
  virtual int GetImage(YV12ImageRP *image, int source = -1, bool readonly = false) = 0;
  virtual void ReleaseImage(int source, bool preserve = false) = 0;
  virtual void AddRPVideoPictureHW(RPVideoPicture &picture, int index) {};
  virtual bool IsPictureHW(RPVideoPicture &picture) { return false; };
  virtual void FlipPage(int source) = 0;
  virtual void PreInit() = 0;
  virtual void UnInit() = 0;
  virtual void Reset() = 0;
  virtual void Flush() {};
  virtual void SetBufferSize(int numBuffers) { }
  virtual void ReleaseBuffer(int idx) { }
  virtual bool NeedBuffer(int idx) { return false; }
  virtual bool IsGuiLayer() { return true; }
  // Render info, can be called before configure
  virtual CRPRenderInfo GetRenderInfo() { return CRPRenderInfo(); }
  virtual void Update() = 0;
  virtual void RenderUpdate(bool clear, unsigned int flags = 0, unsigned int alpha = 255) = 0;
  virtual bool HandlesRenderFormat(ERPRenderFormat format) { return format == m_format; };
  virtual bool ConfigChanged(void *hwPic) { return false; };

  // Feature support
  virtual bool SupportsMultiPassRendering() = 0;
  virtual bool Supports(ERENDERFEATURE feature) { return false; };
  virtual bool Supports(ESCALINGMETHOD method) = 0;

  virtual bool WantsDoublePass() { return false; };

  void SetViewMode(int viewMode);

  /*! \brief Get video rectangle and view window
  \param source is original size of the video
  \param dest is the target rendering area honoring aspect ratio of source
  \param view is the entire target rendering area for the video (including black bars)
  */
  void GetVideoRect(CRect &source, CRect &dest, CRect &view);
  float GetAspectRatio() const;

protected:
  void CalcNormalRenderRect(float offsetX, float offsetY, float width, float height,
                            float inputFrameRatio, float zoomAmount, float verticalShift);
  void CalculateFrameAspectRatio(unsigned int desired_width, unsigned int desired_height);
  void ManageRenderArea();
  virtual void ReorderDrawPoints();//might be overwritten (by egl e.x.)
  virtual ERPShaderFormat GetShaderFormat(ERPRenderFormat renderFormat);
  void saveRotatedCoords();//saves the current state of m_rotatedDestCoords
  void syncDestRectToRotatedPoints();//sync any changes of m_destRect to m_rotatedDestCoords
  void restoreRotatedCoords();//restore the current state of m_rotatedDestCoords from saveRotatedCoords
  void MarkDirty();

  unsigned int m_sourceWidth;
  unsigned int m_sourceHeight;
  float m_sourceFrameRatio;
  float m_fps;

  unsigned int m_renderOrientation; // orientation of the video in degrees counter clockwise
  unsigned int m_oldRenderOrientation; // orientation of the previous frame
  // for drawing the texture with glVertex4f (holds all 4 corner points of the destination rect
  // with correct orientation based on m_renderOrientation
  // 0 - top left, 1 - top right, 2 - bottom right, 3 - bottom left
  CPoint m_rotatedDestCoords[4];
  CPoint m_savedRotatedDestCoords[4];//saved points from saveRotatedCoords call

  CRect m_destRect;
  CRect m_oldDestRect; // destrect of the previous frame
  CRect m_sourceRect;
  CRect m_viewRect;

  // rendering flags
  unsigned m_iFlags;
  ERPRenderFormat m_format = RP_RENDER_FMT_NONE;
};
