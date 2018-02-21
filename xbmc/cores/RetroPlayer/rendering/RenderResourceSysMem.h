/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IRenderResourceMemory.h"

#include <stdint.h>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CRenderResourceSysMem : public IRenderResourceMemory
  {
  public:
    CRenderResourceSysMem(size_t size);
    ~CRenderResourceSysMem() override = default;

    // Implementation of IRenderResource via IRenderResourceMemory
    bool CreateResource() override;
    void DestroyResource() override;

    // Implementation of IRenderResourceMemory
    virtual uint8_t *GetMemory() override;
    virtual size_t GetSize() const override { return m_size; }
    virtual void ReleaseMemory() override { }

  private:
    const size_t m_size;
    std::vector<uint8_t> m_data;
  };
}
}
