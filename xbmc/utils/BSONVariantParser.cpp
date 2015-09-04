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

#include "BSONVariantParser.h"
#include "Base64.h"
#include "system.h" // include before bson.h
#include <bson.h>

CVariant CBSONVariantParser::Parse(const char *bsonData, unsigned int length)
{
  CVariant output;

  bson_t document;
  if (bson_init_static(&document, reinterpret_cast<const uint8_t*>(bsonData), length))
  {
    bson_iter_t it;
    if (bson_iter_init(&it, &document))
    {
      while (bson_iter_next(&it))
        output[bson_iter_key(&it)] = InternalParse(&it);
    }
  }

  return output;
}

CVariant CBSONVariantParser::ParseBase64(const std::string &bsonBase64)
{
  std::string bsonDecoded;
  Base64::Decode(bsonBase64, bsonDecoded);
  return Parse(bsonDecoded.c_str(), bsonDecoded.length());
}

CVariant CBSONVariantParser::InternalParse(void *iterator)
{
  CVariant output;

  bson_iter_t *it = static_cast<bson_iter_t*>(iterator);
  if (BSON_ITER_HOLDS_INT32(it))
    output = CVariant(bson_iter_int32(it));
  else if (BSON_ITER_HOLDS_INT64(it))
    output = CVariant(bson_iter_int64(it));
  else if (BSON_ITER_HOLDS_BOOL(it))
    output = CVariant(bson_iter_bool(it));
  else if (BSON_ITER_HOLDS_DOUBLE(it))
    output = CVariant(bson_iter_double(it));
  else if (BSON_ITER_HOLDS_UTF8(it))
    output = CVariant(bson_iter_utf8(it, NULL));
  else if (BSON_ITER_HOLDS_ARRAY(it))
  {
    bson_iter_t child;
    if (bson_iter_recurse(it, &child))
    {
      while (bson_iter_next(&child))
        output.push_back(InternalParse(&child));
    }
  }
  else if (BSON_ITER_HOLDS_DOCUMENT(it))
  {
    bson_iter_t child;
    if (bson_iter_recurse(it, &child))
    {
      while (bson_iter_next(&child))
        output[bson_iter_key(&child)] = InternalParse(&child);
    }
  }

  return output;
}
