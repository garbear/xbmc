/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "smarthome/utils/RosNameUtils.h"

#include <gtest/gtest.h>

using namespace KODI::SMART_HOME;

TEST(TestRosNameUtils, NormalizeHostname)
{
  EXPECT_EQ("media_room", NormalizeRosHostname("media-room.local"));
  EXPECT_EQ("media_room", NormalizeRosHostname("media-room.LOCAL."));
  EXPECT_EQ("media_room", NormalizeRosHostname("media-room"));
  EXPECT_EQ("media.localhost", NormalizeRosHostname("media.localhost"));
}

TEST(TestRosNameUtils, NormalizeNamespace)
{
  EXPECT_EQ("/", NormalizeRosNamespace(""));
  EXPECT_EQ("/", NormalizeRosNamespace("/"));
  EXPECT_EQ("/oasis", NormalizeRosNamespace("oasis"));
  EXPECT_EQ("/oasis", NormalizeRosNamespace("/oasis"));
  EXPECT_EQ("/oasis", NormalizeRosNamespace("/oasis/"));
  EXPECT_EQ("/home/oasis", NormalizeRosNamespace("home/oasis"));
  EXPECT_EQ("/home/oasis", NormalizeRosNamespace("/home/oasis/"));
}

TEST(TestRosNameUtils, BuildName)
{
  EXPECT_EQ("/station/input", BuildRosName("", {"station", "input"}));
  EXPECT_EQ("/station/input", BuildRosName("/", {"station", "input"}));
  EXPECT_EQ("/home/oasis/station/input", BuildRosName("/home/oasis/", {"/station/", "/input/"}));
  EXPECT_EQ("/oasis/station/input", BuildRosName("oasis", {"station", "input"}));
}

TEST(TestRosNameUtils, GeneratedNamesNeverHaveDoubleLeadingSlash)
{
  for (const std::string rosNamespace : {"", "/", "oasis", "/oasis", "/home/oasis/"})
  {
    const std::string name = BuildRosName(rosNamespace, {"/station/", "/input/"});
    EXPECT_NE(0, name.rfind("//", 0));
  }
}
