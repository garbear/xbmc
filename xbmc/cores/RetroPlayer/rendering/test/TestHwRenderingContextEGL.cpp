/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/contexts/HwRenderingContextEGLUtils.h"

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestHwRenderingContextEGL, EGL14RequiresBothExtensions)
{
  EXPECT_FALSE(
      SupportsEGLHardwareRendering("1.3", "EGL_KHR_surfaceless_context EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", ""));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_surfaceless_context"));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.4 vendor",
                                           "EGL_KHR_surfaceless_context EGL_KHR_create_context"));
}

TEST(TestHwRenderingContextEGL, EGL15DoesNotRequireExtensionStrings)
{
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", ""));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5 vendor", nullptr));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.10", ""));
  EXPECT_TRUE(SupportsEGLHardwareRendering("2.0", ""));
}

TEST(TestHwRenderingContextEGL, FailedQueriesAndMalformedVersionsAreRejected)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering(nullptr, nullptr));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", nullptr));
  for (const char* version : {"", "garbage", "1", "1.", "1.5junk", "-1.5", "1.999999999999999"})
    EXPECT_FALSE(
        SupportsEGLHardwareRendering(version, "EGL_KHR_surfaceless_context EGL_KHR_create_context"))
        << version;
}

TEST(TestHwRenderingContextEGL, ExtensionNamesMustBeCompleteTokens)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context_extra EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context not_EGL_KHR_create_context"));
  EXPECT_TRUE(SupportsEGLHardwareRendering(
      "1.4",
      " EGL_KHR_surfaceless_context_extra EGL_KHR_create_context  EGL_KHR_surfaceless_context "));
}
