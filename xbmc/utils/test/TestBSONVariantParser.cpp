/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/BSONVariantParser.h"

#include "gtest/gtest.h"

TEST(TestBSONVariantParser, Parse)
{
  // http://bsonspec.org/#/specification

  const char helloWorldIn[] = "\x16\x00\x00\x00\x02hello\x00\x06\x00\x00\x00world\x00\x00";

  CVariant helloWorldOut = CBSONVariantParser::Parse(helloWorldIn, sizeof(helloWorldIn) - 1);

  EXPECT_TRUE(helloWorldOut.isObject());
  EXPECT_EQ(1, helloWorldOut.size());
  EXPECT_TRUE(helloWorldOut["hello"].isString());
  EXPECT_STREQ("world", helloWorldOut["hello"].asString().c_str());

  const char heterogeneousIn[] = "\x31\x00\x00\x00\x04" "BSON\x00\x26\x00\x00\x00\x02" "0"
                                 "\x00\x08\x00\x00\x00" "awesome\x00\x01" "1\x00\x33\x33\x33"
                                 "\x33\x33\x33\x14\x40\x10" "2\x00\xc2\x07\x00\x00\x00\x00";

  CVariant heterogeneousOut = CBSONVariantParser::Parse(heterogeneousIn, sizeof(heterogeneousIn) - 1);

  EXPECT_TRUE(heterogeneousOut.isObject());
  EXPECT_EQ(1, heterogeneousOut.size());
  EXPECT_TRUE(heterogeneousOut["BSON"].isArray());
  if (heterogeneousOut["BSON"].isArray())
  {
    CVariant arrayOut = heterogeneousOut["BSON"];
    EXPECT_STREQ("awesome", arrayOut[0].asString().c_str());
    EXPECT_EQ(5.05, arrayOut[1].asDouble());
    EXPECT_EQ(1986, arrayOut[2].asInteger());
  }
}
