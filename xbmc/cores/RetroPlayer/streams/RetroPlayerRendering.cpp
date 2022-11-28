/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "RetroPlayerRendering.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerRendering::CRetroPlayerRendering(CRPRenderManager& renderManager,
                                             CRPProcessInfo& processInfo)
  : m_renderManager(renderManager), m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Initializing rendering");
}

CRetroPlayerRendering::~CRetroPlayerRendering()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Deinitializing rendering");

  CloseStream();
  m_renderManager.Deinitialize();
}

bool CRetroPlayerRendering::OpenStream(const StreamProperties& properties)
{
  const RenderingStreamProperties& renderingProperties =
      static_cast<const RenderingStreamProperties&>(properties);

  const AVPixelFormat pixfmt = renderingProperties.pixfmt;
  const unsigned int nominalWidth = renderingProperties.nominalWidth;
  const unsigned int nominalHeight = renderingProperties.nominalHeight;
  const unsigned int maxWidth = renderingProperties.maxWidth;
  const unsigned int maxHeight = renderingProperties.maxHeight;
  const float pixelAspectRatio = renderingProperties.pixelAspectRatio; //! @todo

  CLog::Log(
      LOGDEBUG,
      "RetroPlayer[RENDERING]: Creating rendering stream - format {}, nominal {}x{}, max {}x{}",
      CRenderTranslator::TranslatePixelFormat(pixfmt), nominalWidth, nominalHeight, maxWidth,
      maxHeight);

  m_processInfo.SetVideoPixelFormat(pixfmt);
  m_processInfo.SetVideoDimensions(nominalWidth, nominalHeight); // Report nominal height for now

  if (!m_renderManager.Configure(pixfmt, nominalWidth, nominalHeight, maxWidth, maxHeight,
                                 pixelAspectRatio))
    return false;

  return m_renderManager.Create(maxWidth, maxHeight);
}

void CRetroPlayerRendering::CloseStream()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Closing rendering stream");

  //! @todo
}

bool CRetroPlayerRendering::GetStreamBuffer(unsigned int width,
                                            unsigned int height,
                                            StreamBuffer& buffer)
{
  HwFramebufferBuffer& hwBuffer = static_cast<HwFramebufferBuffer&>(buffer);

  hwBuffer.framebuffer = m_renderManager.GetCurrentFramebuffer(width, height);

  return true;
}

void CRetroPlayerRendering::AddStreamData(const StreamPacket& packet)
{
  // This is left here in case anything gets added to the api in the future
  [[maybe_unused]] const HwFramebufferPacket& hwPacket =
      static_cast<const HwFramebufferPacket&>(packet);

  m_renderManager.RenderFrame();
}
