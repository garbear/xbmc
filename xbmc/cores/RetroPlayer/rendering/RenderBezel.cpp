/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBezel.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "guilib/GUITexture.h"
#include "guilib/Texture.h"
#include "utils/ColorUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

namespace
{
// Alpha from which a pixel belongs to the frame
constexpr uint8_t OPAQUE_ALPHA = 250;

// How far above the centre's alpha a pixel in the window is still glass
constexpr uint8_t GLASS_TOLERANCE = 32;

// Least the game may shrink to fit the window
constexpr float MIN_GAME_SCALE = 0.8f;

// Least of the window the game must cover
constexpr float MIN_WINDOW_COVERAGE = 0.75f;

// Byte offset of alpha in a 32-bit pixel
constexpr unsigned int ALPHA_BYTE = 3;
} // namespace

CRenderBezel::CRenderBezel(std::unique_ptr<CTexture> texture, const CRect& window)
  : m_texture(std::move(texture)),
    m_width(m_texture->GetWidth()),
    m_height(m_texture->GetHeight()),
    m_window(window)
{
}

CRenderBezel::~CRenderBezel() = default;

std::unique_ptr<CRenderBezel> CRenderBezel::Load(const std::string& url)
{
  CTextureCache& textureCache = *CServiceBroker::GetTextureCache();

  std::unique_ptr<CTexture> texture;

  bool needsRecaching = false;
  const std::string cachedPath = textureCache.CheckCachedImage(url, needsRecaching);
  if (!cachedPath.empty())
  {
    texture = CTexture::LoadFromFile(cachedPath);
    if (texture && needsRecaching)
      textureCache.BackgroundCacheImage(url);
  }

  if (!texture)
    textureCache.CacheImage(url, &texture);

  // Bezel URLs can carry a scraper's credentials, so they are never logged
  if (!texture || texture->GetPixels() == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to load bezel");
    return {};
  }

  CRect window;
  if (!FindWindow(texture->GetPixels(), texture->GetWidth(), texture->GetHeight(),
                  texture->GetPitch(), window))
  {
    CLog::Log(LOGINFO, "RetroPlayer[RENDER]: Bezel has no window for the game, not showing it");
    return {};
  }

  ClearWindow(texture->GetPixels(), texture->GetWidth(), texture->GetHeight(), texture->GetPitch(),
              window);

  CLog::Log(LOGDEBUG,
            "RetroPlayer[RENDER]: Loaded {}x{} bezel, window ({:.0f},{:.0f})-({:.0f},{:.0f})",
            texture->GetWidth(), texture->GetHeight(), window.x1, window.y1, window.x2, window.y2);

  return std::unique_ptr<CRenderBezel>(new CRenderBezel(std::move(texture), window));
}

CRect CRenderBezel::GetBezelRect(const CRect& screen) const
{
  const float scale = std::min(screen.Width() / static_cast<float>(m_width),
                               screen.Height() / static_cast<float>(m_height));
  const float width = m_width * scale;
  const float height = m_height * scale;
  const float x = screen.x1 + (screen.Width() - width) / 2;
  const float y = screen.y1 + (screen.Height() - height) / 2;

  return {x, y, x + width, y + height};
}

CRect CRenderBezel::GetWindowRect(const CRect& screen) const
{
  const CRect bezel = GetBezelRect(screen);
  const float scale = bezel.Width() / static_cast<float>(m_width);

  return {bezel.x1 + m_window.x1 * scale, bezel.y1 + m_window.y1 * scale,
          bezel.x1 + m_window.x2 * scale, bezel.y1 + m_window.y2 * scale};
}

void CRenderBezel::Render(const CRect& screen)
{
  // The texture's rows are padded past the picture's width
  const CRect textureCoords(0.0f, 0.0f, static_cast<float>(m_width) / m_texture->GetTextureWidth(),
                            static_cast<float>(m_height) / m_texture->GetTextureHeight());

  CGUITexture::DrawQuad(GetBezelRect(screen), UTILS::COLOR::WHITE, m_texture.get(), &textureCoords);
}

bool CRenderBezel::FindWindow(const uint8_t* pixels,
                              unsigned int width,
                              unsigned int height,
                              unsigned int pitch,
                              CRect& window)
{
  if (pixels == nullptr || width == 0 || height == 0)
    return false;

  auto IsWindow = [pixels, pitch](unsigned int x, unsigned int y)
  { return pixels[y * pitch + x * 4 + ALPHA_BYTE] < OPAQUE_ALPHA; };

  const unsigned int centreX = width / 2;
  const unsigned int centreY = height / 2;
  if (!IsWindow(centreX, centreY))
    return false;

  unsigned int left = centreX;
  while (left > 0 && IsWindow(left - 1, centreY))
    left--;

  unsigned int right = centreX + 1;
  while (right < width && IsWindow(right, centreY))
    right++;

  unsigned int top = centreY;
  while (top > 0 && IsWindow(centreX, top - 1))
    top--;

  unsigned int bottom = centreY + 1;
  while (bottom < height && IsWindow(centreX, bottom))
    bottom++;

  window = CRect(static_cast<float>(left), static_cast<float>(top), static_cast<float>(right),
                 static_cast<float>(bottom));

  return true;
}

void CRenderBezel::ClearWindow(uint8_t* pixels,
                               unsigned int width,
                               unsigned int height,
                               unsigned int pitch,
                               const CRect& window)
{
  if (pixels == nullptr || width == 0 || height == 0)
    return;

  // Glass is flat, so whatever is near the centre's opacity is glass and
  // whatever is above it is the frame's edge
  const unsigned int centreAlpha = pixels[(height / 2) * pitch + (width / 2) * 4 + ALPHA_BYTE];
  const unsigned int glassAlpha = centreAlpha + GLASS_TOLERANCE;

  const unsigned int x2 = std::min(static_cast<unsigned int>(window.x2), width);
  const unsigned int y2 = std::min(static_cast<unsigned int>(window.y2), height);

  for (unsigned int y = static_cast<unsigned int>(window.y1); y < y2; y++)
  {
    uint8_t* row = pixels + y * pitch;
    for (unsigned int x = static_cast<unsigned int>(window.x1); x < x2; x++)
    {
      uint8_t& alpha = row[x * 4 + ALPHA_BYTE];
      if (alpha <= glassAlpha)
        alpha = 0;
    }
  }
}

bool CRenderBezel::IsShownInStretchMode(STRETCHMODE stretchMode)
{
  switch (stretchMode)
  {
    case STRETCHMODE::Stretch16x9:
    case STRETCHMODE::Fullscreen:
      return false;
    default:
      break;
  }

  return true;
}

bool CRenderBezel::FramesGame(const CRect& window,
                              const CRect& gameInWindow,
                              const CRect& gameOnScreen)
{
  if (window.IsEmpty() || gameInWindow.IsEmpty() || gameOnScreen.IsEmpty())
    return false;

  // Allow for the game's position being rounded to whole pixels
  CRect bounds(window);
  bounds.x1 -= 1.0f;
  bounds.y1 -= 1.0f;
  bounds.x2 += 1.0f;
  bounds.y2 += 1.0f;
  if (gameInWindow.x1 < bounds.x1 || gameInWindow.y1 < bounds.y1 || gameInWindow.x2 > bounds.x2 ||
      gameInWindow.y2 > bounds.y2)
    return false;

  if (gameInWindow.Width() < MIN_GAME_SCALE * gameOnScreen.Width() ||
      gameInWindow.Height() < MIN_GAME_SCALE * gameOnScreen.Height())
    return false;

  return gameInWindow.Area() >= MIN_WINDOW_COVERAGE * window.Area();
}
