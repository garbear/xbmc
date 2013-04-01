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

#include "utils/BSONVariantWriter.h"
#include "utils/Variant.h"
#include "gtest/gtest.h"

#include <cstring>

TEST(TestBSONVariantWriter, Write)
{
  // http://bsonspec.org/#/specification

  CVariant helloWorldIn;
  helloWorldIn["hello"] = "world";
  const char helloWorldRef[] = "\x16\x00\x00\x00\x02hello\x00\x06\x00\x00\x00world\x00\x00";

  std::string helloWorldOut = CBSONVariantWriter::Write(helloWorldIn);
  EXPECT_EQ(0, memcmp(helloWorldOut.c_str(), helloWorldRef, helloWorldOut.size()));

  CVariant heterogeneousIn, arrayIn;
  arrayIn.push_back("awesome");
  arrayIn.push_back(5.05);
  arrayIn.push_back(1986);
  heterogeneousIn["BSON"] = arrayIn;
  // Reference is different than heterogeneousIn in TestBSONVariantParser.cpp.
  // It has been adapted to use 1986 as a long. The size is 4 bytes longer
  // (0x35 instead of 0x31), it's padded with four zeros, the type (indicated
  // by the ^) has been changed from int32 (0x10) to int64 (0x12), and the
  // array length has been increased by 4 from 0x26 to 0x2a (here..  v).
  const char heterogeneousRef[] = "\x35\x00\x00\x00\x04" "BSON\x00\x2a\x00\x00\x00\x02" "0"
                                  "\x00\x08\x00\x00\x00" "awesome\x00\x01" "1\x00\x33\x33\x33"
                                  "\x33\x33\x33\x14\x40\x12" "2\x00\xc2\x07\x00\x00\x00\x00"
                                  "\x00\x00\x00\x00";  // ^

  std::string heterogeneousOut = CBSONVariantWriter::Write(heterogeneousIn);
  EXPECT_EQ(0, memcmp(heterogeneousOut.c_str(), heterogeneousRef, heterogeneousOut.size()));
}
