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

#include "BSONVariantWriter.h"
#include "Base64.h"
#include "system.h" // include before bson.h
#include <bson.h>
#include "log.h"
#include "Variant.h"

void CBSONVariantWriter::WriteBase64(const CVariant &value, std::string &output)
{
  std::string data;
  CBSONVariantWriter::Write(value, data);
  Base64::Encode(data.data(), data.size(), output);
}

std::string CBSONVariantWriter::WriteBase64(const CVariant &value)
{
  std::string data;
  CBSONVariantWriter::Write(value, data);
  return Base64::Encode(data.data(), data.size());
}

std::string CBSONVariantWriter::Write(const CVariant &value)
{
  std::string output;
  Write(value, output);
  return output;
}

void CBSONVariantWriter::Write(const CVariant &value, std::string &output)
{
  output.clear();

  bson_t document = BSON_INITIALIZER;

  bool success = true;
  for (CVariant::const_iterator_map it = value.begin_map(); it != value.end_map() && success; it++)
    success &= InternalWrite(&document, it->first.c_str(), it->second);

  output.assign(bson_get_data(&document), bson_get_data(&document) + document.len);

  bson_destroy(&document);
}

bool CBSONVariantWriter::InternalWrite(bson_t *document, const char *name, const CVariant &value)
{
  bool success;

  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    success = BSON_APPEND_INT64(document, name, value.asInteger());
    break;
  case CVariant::VariantTypeUnsignedInteger:
    success = BSON_APPEND_INT64(document, name, (int64_t)value.asUnsignedInteger());
    break;
  case CVariant::VariantTypeDouble:
    success = BSON_APPEND_DOUBLE(document, name, value.asDouble());
    break;
  case CVariant::VariantTypeBoolean:
    success = BSON_APPEND_BOOL(document, name, value.asBoolean());
    break;
  case CVariant::VariantTypeString:
    success = BSON_APPEND_UTF8(document, name, value.asString().c_str());
    break;
  case CVariant::VariantTypeArray:
    {
      bson_t child;
      success = BSON_APPEND_ARRAY_BEGIN(document, name, &child);

      // For the array, we have to manually set the index values from "0" to "N"
      unsigned short i = 0;
      char index[6];
      for (CVariant::const_iterator_array it = value.begin_array(); it != value.end_array() && success; it++)
      {
        snprintf(index, sizeof(index), "%u", i++);
        success &= InternalWrite(&child, index, *it);
      }
      success &= bson_append_array_end(document, &child);
    }
    break;
  case CVariant::VariantTypeObject:
    {
      bson_t child;
      success = BSON_APPEND_DOCUMENT_BEGIN(document, name, &child);

      for (CVariant::const_iterator_map it = value.begin_map(); it != value.end_map() && success; it++)
        success &= InternalWrite(&child, it->first.c_str(), it->second);

      success &= bson_append_document_end(document, &child);

      break;
    }
  case CVariant::VariantTypeNull:
  case CVariant::VariantTypeConstNull:
  default:
    success = BSON_APPEND_NULL(document, name);
    break;
  }

  return success;
}
