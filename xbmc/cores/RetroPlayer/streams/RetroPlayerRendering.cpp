/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerRendering.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerRendering::CRetroPlayerRendering(CRPRenderManager& renderManager,
                                             CRPProcessInfo& processInfo)
  : m_renderManager(renderManager),
    m_processInfo(processInfo),
    m_width(640), //! @todo
    m_height(480) //! @todo
{
}

bool CRetroPlayerRendering::OpenStream(const StreamProperties& properties)
{
  m_processInfo.SetVideoPixelFormat(AV_PIX_FMT_BGRA);
  m_processInfo.SetVideoDimensions(m_width, m_height);

  if (!m_renderManager.Configure(AV_PIX_FMT_BGRA, m_width, m_height, m_width, m_height, 1.0f))
    return false;

  return m_renderManager.Create();
}

void CRetroPlayerRendering::CloseStream()
{
  //! @todo
}

bool CRetroPlayerRendering::GetStreamBuffer(unsigned int width,
                                            unsigned int height,
                                            StreamBuffer& buffer)
{
  HwFramebufferBuffer& hwBuffer = static_cast<HwFramebufferBuffer&>(buffer);

  hwBuffer.framebuffer = m_renderManager.GetCurrentFramebuffer();

  return true;
}

void CRetroPlayerRendering::AddStreamData(const StreamPacket& packet)
{
  m_renderManager.RenderFrame();
}
