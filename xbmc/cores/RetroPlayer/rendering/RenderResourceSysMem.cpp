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

#include "RenderResourceSysMem.h"

using namespace KODI;
using namespace RETRO;

CRenderResourceSysMem::CRenderResourceSysMem(size_t size) :
  m_size(size)
{
}

bool CRenderResourceSysMem::CreateResource()
{
  m_data.resize(m_size);
  return true;
}

void CRenderResourceSysMem::DestroyResource()
{
  m_data.clear();
}

uint8_t *CRenderResourceSysMem::GetMemory()
{
  return const_cast<uint8_t*>(m_data.data());
}
