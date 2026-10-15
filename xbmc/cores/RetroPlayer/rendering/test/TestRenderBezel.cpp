/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/RenderBezel.h"

#include <stdint.h>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
constexpr unsigned int WIDTH = 64;
constexpr unsigned int HEIGHT = 36;

struct Picture
{
  Picture(unsigned int padding = 0) : pitch((WIDTH + padding) * 4), pixels(pitch * HEIGHT, 0xff) {}

  uint8_t& Alpha(unsigned int x, unsigned int y) { return pixels[y * pitch + x * 4 + 3]; }

  void Fill(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, uint8_t alpha)
  {
    for (unsigned int y = y1; y < y2; y++)
      for (unsigned int x = x1; x < x2; x++)
        Alpha(x, y) = alpha;
  }

  unsigned int pitch;
  std::vector<uint8_t> pixels;
};
} // namespace

TEST(TestRenderBezel, FindsClearWindow)
{
  Picture picture;
  picture.Fill(12, 2, 52, 34, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 2, 52, 34), window);
}

TEST(TestRenderBezel, FindsGlassWindow)
{
  Picture picture;
  picture.Fill(12, 2, 52, 34, 128);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 2, 52, 34), window);
}

TEST(TestRenderBezel, FindsWindowInPaddedRows)
{
  Picture picture(16);
  picture.Fill(12, 2, 52, 34, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(12, 2, 52, 34), window);
}

TEST(TestRenderBezel, FindsWindowReachingEdges)
{
  Picture picture;
  picture.Fill(20, 0, 44, HEIGHT, 0);

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  EXPECT_EQ(CRect(20, 0, 44, HEIGHT), window);
}

TEST(TestRenderBezel, OpaquePictureHasNoWindow)
{
  Picture picture;

  CRect window;
  EXPECT_FALSE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
}

TEST(TestRenderBezel, ClearsGlassAndKeepsFrameEdge)
{
  Picture picture;
  picture.Fill(12, 2, 52, 34, 85);
  picture.Fill(12, 2, 14, 34, 200); // soft edge where the frame meets the window

  CRect window;
  ASSERT_TRUE(
      CRenderBezel::FindWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window));
  CRenderBezel::ClearWindow(picture.pixels.data(), WIDTH, HEIGHT, picture.pitch, window);

  EXPECT_EQ(0, picture.Alpha(WIDTH / 2, HEIGHT / 2));
  EXPECT_EQ(0, picture.Alpha(50, 3));
  EXPECT_EQ(200, picture.Alpha(12, 10));
  EXPECT_EQ(255, picture.Alpha(5, 10));
  EXPECT_EQ(255, picture.Alpha(30, 1));
}

TEST(TestRenderBezel, StepsAsideForWideStretchModes)
{
  EXPECT_TRUE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Normal));
  EXPECT_TRUE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Stretch4x3));
  EXPECT_TRUE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Integer));
  EXPECT_TRUE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Original));
  EXPECT_FALSE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Stretch16x9));
  EXPECT_FALSE(CRenderBezel::IsShownInStretchMode(STRETCHMODE::Fullscreen));
}

TEST(TestRenderBezel, FramesGameOfWindowShape)
{
  // A console bezel's 4:3 window on a 1080p screen
  const CRect window(259, 18, 1661, 1061);
  EXPECT_TRUE(CRenderBezel::FramesGame(window, {265, 18, 1656, 1061}, {240, 0, 1680, 1080}));
}

TEST(TestRenderBezel, FramesWiderGameWithBars)
{
  // A 3:2 handheld game in a 4:3 window
  const CRect window(260, 2, 1659, 1077);
  EXPECT_TRUE(CRenderBezel::FramesGame(window, {260, 73, 1659, 1006}, {150, 0, 1770, 1080}));
}

TEST(TestRenderBezel, SkipsSmallWindow)
{
  // A photo of a cabinet, its screen a little over half the screen's height
  const CRect window(707, 182, 1238, 875);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {713, 182, 1233, 875}, {555, 0, 1365, 1080}));
}

TEST(TestRenderBezel, SkipsWindowOfWrongShape)
{
  // A game turned on its side, in a window made for it upright
  const CRect window(259, 18, 1661, 1061);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {569, 18, 1351, 1061}, {555, 0, 1365, 1080}));
}

TEST(TestRenderBezel, SkipsGameLargerThanWindow)
{
  const CRect window(259, 18, 1661, 1061);
  EXPECT_FALSE(CRenderBezel::FramesGame(window, {240, 0, 1680, 1080}, {240, 0, 1680, 1080}));
}
