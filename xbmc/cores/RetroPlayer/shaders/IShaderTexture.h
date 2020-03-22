/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace SHADER
{
  class IShaderTexture
  {
  public:
    virtual ~IShaderTexture();

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
}
}
