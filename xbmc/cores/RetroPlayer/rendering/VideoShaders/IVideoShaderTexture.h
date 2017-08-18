#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
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

#include "IVideoShaderPreset.h"
#include "VideoShaderUtils.h"

#include <memory>
#include <string>

namespace KODI
{
namespace SHADER
{
  class IShaderSampler
  {
  public:
    virtual ~IShaderSampler() = default;
  };

  class IShaderTexture
  {
  public:
    virtual ~IShaderTexture() = default;

    /*!
     * \brief Return width of texture
     * \return Width of the texture in texels
     */
    virtual float GetWidth() const = 0;

    /*!
    * \brief Return height of texture
    * \return Height of the texture in texels
    */
    virtual float GetHeight() const = 0;
  };

  class IShaderLut
  {
  public:
    IShaderLut() = default;
    IShaderLut(const std::string& id, const std::string& path)
      : m_id(id), m_path(path) {}

    virtual ~IShaderLut() = default;

    /*!
     * \brief Gets ID of LUT
     * \return Unique name (ID) of look up texture
     */
    const std::string& GetID() const { return m_id; }

    /*!
    * \brief Gets full path of LUT
    * \return Full path of look up texture
    */
    const std::string& GetPath() const { return m_path; }

    /*!
    * \brief Gets sampler of LUT
    * \return Pointer to the sampler associated with the LUT
    */
    virtual IShaderSampler* GetSampler() = 0;

    /*!
    * \brief Gets sampler of LUT
    * \return Pointer to the texture where the LUT data is stored in
    */
    virtual IShaderTexture* GetTexture() = 0;

  protected:
    std::string m_id;
    std::string m_path;
  };

  using IShaderLuts = std::vector<std::shared_ptr<IShaderLut>>;
}
}
