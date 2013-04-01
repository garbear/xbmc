#pragma once
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

#include <string>

class CVariant;
typedef struct _bson_t bson_t;

class CBSONVariantWriter
{
public:
  static std::string Write(const CVariant &value);
  static void        Write(const CVariant &value, std::string &output);
  static std::string WriteBase64(const CVariant &value);
  static void        WriteBase64(const CVariant &value, std::string &output);

private:
  static bool InternalWrite(bson_t *document, const char *name, const CVariant &value);
};
