/*
 *      Copyright (C) 2012 Garrett Brown
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "linux/PlatformDefs.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "DllSwScale.h"
#include "games/libretro/libretro.h"
#include "threads/Thread.h"

#include <stdint.h>
#include <queue>

struct SwsContext;

class CRetroPlayerVideo : public CThread
{
public:
  struct Frame
  {
    std::vector<unsigned char> data;
    unsigned int  width;
    unsigned int  height;
    size_t        pitch;
  };

  CRetroPlayerVideo();
  ~CRetroPlayerVideo();

  /**
   * Begin doing what a RetroPlayerVideo does best.
   */
  void GoForth(double framerate, bool fullscreen);

  void StopThread(bool bWait = true);

  /**
   * Send a video frame to be rendered by this class. The pixel format is
   * specified by m_pixelFormat.
   */
  void SendVideoFrame(const void *data, unsigned width, unsigned height, size_t pitch);

  /**
   * Set the m_pixelFormat to match the format used by the game client. If this
   * function is not called, m_pixelFormat defaults to RETRO_PIXEL_FORMAT_0RGB1555.
   * For a list of valid pixel formats, see libretro.h.
   */
  void SetPixelFormat(retro_pixel_format pixelFormat) { m_pixelFormat = pixelFormat; }

  /**
   * Small bit of trivia: IPlayer::Update() is unused and can be removed.
   */
  virtual void Update(bool bPauseDrawing) { g_renderManager.Update(bPauseDrawing); }

protected:
  virtual void Process();

private:
  bool CheckConfiguration(const DVDVideoPicture &picture);

  Frame              m_queuedFrame;
  CEvent             m_frameEvent;  // Set immediately when a new frame is queued
  CCriticalSection   m_critSection; // Guard m_queuedFrame
  retro_pixel_format m_pixelFormat;
  DllSwScale         m_dllSwScale;
  SwsContext         *m_swsContext;

  bool               m_bAllowFullscreen;
  double             m_framerate;
  // Output variables are used to store the current output configuration. Each
  // frame the picture's configuration is compared to the output configuration,
  // and a discrepancy will reconfigure the render manager, as well as saving
  // the new state to these variables.
  unsigned int       m_outputWidth;
  unsigned int       m_outputHeight;
  double             m_outputFramerate;
};
