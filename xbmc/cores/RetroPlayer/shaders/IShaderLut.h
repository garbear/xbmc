/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{

namespace RETRO
{
  class CRenderContext;
}

namespace SHADER
{
  class IShaderSampler;
  class IShaderTexture;

  /*!
   * \brief A lookup table to apply color transforms in a shader
   */
  class IShaderLut
  {
  public:
    IShaderLut() = default;
    IShaderLut(const std::string& id, const std::string& path)
      : m_id(id), m_path(path) {}

    virtual ~IShaderLut() = default;

    /*!
     * \brief Create the LUT and allocate resources
     * \param context The render context
     * \param lut The LUT information structure
     * \return True if successful and the LUT can be used, false otherwise
     */
    virtual bool Create(RETRO::CRenderContext &context, const ShaderLut &lut) = 0;

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
}
}

