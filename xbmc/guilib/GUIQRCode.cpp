/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIQRCode.h"

#include "FileItem.h"
#include "GUITexture.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "utils/QRUtils.h"

#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#include "utils/GLUtils.h"

#include "system_gl.h"
#elif HAS_GLES >= 2
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"

#include "system_gl.h"
#elif defined(TARGET_WINDOWS)
#include "guilib/GUIShaderDX.h"
#include "guilib/TextureDX.h"
#include "rendering/dx/RenderContext.h"
//#include "rendering/dx/RenderSystemDX.h"

#include <DirectXMath.h>
using namespace DirectX;
#endif

using namespace KODI;
using namespace GUILIB;

CGUIQRCode::CGUIQRCode(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_QRCODE;
}

CGUIQRCode::CGUIQRCode(const CGUIQRCode& other)
  : CGUIControl(other), m_contentInfo(other.m_contentInfo)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_QRCODE;
}

CGUIQRCode::~CGUIQRCode() = default;

CGUIQRCode* CGUIQRCode::Clone() const
{
  return new CGUIQRCode(*this);
}

void CGUIQRCode::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  /*
  if (m_texture->Process(currentTime))
    MarkDirtyRegion();
  */

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIQRCode::Render()
{
  if (!IsVisible())
    return;

  if (!m_texture)
    return;

  float x = GetXPosition();
  float y = GetYPosition();
  float width = GetWidth();
  float height = GetHeight();

  // Center according to 1.0 aspect ratio
  if (width > height)
  {
    width = height;
    x += (GetWidth() - width) / 2.0f;
  }
  else if (height > width)
  {
    height = width;
    y += (GetHeight() - height) / 2.0f;
  }

  std::array<CPoint, 4> destCoords;
  // Top left
  destCoords[0] = CPoint{CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(x, y),
                         CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(x, y)};
  // Top right
  destCoords[1] =
      CPoint{CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(x + width, y),
             CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(x + width, y)};
  // Bottom right
  destCoords[2] = CPoint{
      CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(x + width, y + height),
      CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(x + width, y + height)};
  // Bottom left
  destCoords[3] =
      CPoint{CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(x, y + height),
             CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(x, y + height)};

#if defined(HAS_DX)
  const uint32_t color = 0xFFFFFFFF;

  Vertex vertex[4];
  for (int i = 0; i < 4; i++)
  {
    vertex[i].pos = XMFLOAT3(destCoords[i].x, destCoords[i].y, 0);
    CD3DHelper::XMStoreColor(&vertex[i].color, color);
    vertex[i].texCoord = XMFLOAT2(0.0f, 0.0f);
    vertex[i].texCoord2 = XMFLOAT2(0.0f, 0.0f);
  }

  vertex[1].texCoord.x = vertex[2].texCoord.x = 1.0f;
  vertex[2].texCoord.y = vertex[3].texCoord.y = 1.0f;

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  if (pGUIShader != nullptr)
  {
    pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND_NEAREST);

    // Set state to render the image
    auto dxTexture = static_cast<CDXTexture*>(m_texture.get());
    ID3D11ShaderResourceView* shaderRes = dxTexture->GetShaderResource();
    pGUIShader->SetShaderViews(1, &shaderRes);
    pGUIShader->DrawQuad(vertex[0], vertex[1], vertex[2], vertex[3]);
  }
#endif

  CGUIControl::Render();
}

void CGUIQRCode::SetContent(const GUIINFO::CGUIInfoLabel& infoLabel)
{
  m_contentInfo = infoLabel;

  // Check if content is available without a listitem
  CFileItem empty;
  const std::string strContent = m_contentInfo.GetItemLabel(&empty);
  if (!strContent.empty())
    UpdateContent(strContent);
}

void CGUIQRCode::UpdateContent(const std::string& content)
{
  std::vector<bool> pixels;
  unsigned int width;
  if (UTILS::CQRUtils::EncodeString(content, pixels, width))
  {
    const unsigned int height = width;

    // Convert to AV_PIX_FMT_ARGB
    std::vector<uint8_t> argbPixels;
    argbPixels.reserve(pixels.size() * 4);
    for (const bool pixel : pixels)
    {
      argbPixels.push_back(pixel ? 0 : 0xff);
      argbPixels.push_back(pixel ? 0 : 0xff);
      argbPixels.push_back(pixel ? 0 : 0xff);
      argbPixels.push_back(0xff);
    }

    // Create texture
    m_texture = CTexture::CreateTexture(width, height, XB_FMT_A8R8G8B8);
    if (m_texture)
    {
      m_texture->SetScalingMethod(TEXTURE_SCALING::NEAREST);
      m_texture->LoadFromMemory(width, height, width * 4, XB_FMT_A8R8G8B8, true, argbPixels.data());
      m_texture->LoadToGPU();
    }
  }
}
