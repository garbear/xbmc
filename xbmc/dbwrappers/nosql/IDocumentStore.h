/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
#pragma once

class CVariant;

namespace dbiplus
{
  class IDocumentStore
  {
  public:
    virtual ~IDocumentStore(void) { }

    virtual bool Connect(void) = 0;
    virtual void Disconnect(void) = 0;

    virtual bool Put(const CVariant& key, const CVariant& value) = 0;
    virtual bool Get(const CVariant& key, CVariant& value) = 0;
    virtual bool Delete(const CVariant& key) = 0;
  };
}
