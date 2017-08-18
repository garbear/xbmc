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

#include "IVideoShaderTexture.h"
#include "utils/Geometry.h"

#include <string>
#include <stdint.h>

namespace KODI
{
namespace SHADER
{
  struct float2;
  class IShaderSampler;
  class IShaderTexture;

  class IVideoShader
  {
  public:
    /*!
     * \brief Construct the vidoe shader instance
     * \param shaderSource Source code of the shader (both vertex and pixel/fragment)
     * \param shaderPath Full path to the shader file
     * \param shaderParameters Struct with all parameters pertaining to the shader
     * \param sampler Pointer to the sampler that the will be used when sampling textures
     * \param luts Look-up textures pertaining to shader
     * \param viewPortSize Size of the window/viewport
     * \param frameCountMod Modulo that must be applied to the frame count before sendign it to the shader
     * \return False if creating the shader failed, true otherwise.
     */
    virtual bool Create(const std::string& shaderSource, const std::string& shaderPath, ShaderParameters shaderParameters,
      IShaderSampler* sampler, IShaderLuts luts, float2 viewPortSize, unsigned frameCountMod = 0) = 0;

    /*!
     * \brief Renders the video shader to the target texture
     * \param source Source texture to pass to the shader as input
     * \param target Target texture to render the shader to
     */
    virtual void Render(IShaderTexture* source, IShaderTexture* target) = 0;

    /*!
     * \brief Sets the input and output sizes in pixels
     * \param prevSize Input image size of the shader in pixels
     * \param nextSize Output image size of the shader in pixels
     */
    virtual void SetSizes(const float2& prevSize, const float2& nextSize) = 0;

    /*!
     * \brief Construct the vertex buffer that will be used to render the shader
     * \param vertCount Number of vertices to construct. Commonly 4, for rectangular screens.
     * \param vertSize Size of each vertex's data in bytes
     * \return False if creating the vertex buffer failed, true otherwise.
     */
    virtual bool CreateVertexBuffer(unsigned vertCount, unsigned vertSize) = 0;

    /*!
     * \brief Creates the data layout of the input-assembler stage
     * \param layout Description of the inputs to the vertex shader
     * \param numElements Number of inputs to the vertex shader
     * \return False if creating the input layout failed, true otherwise.
     */
    // TODO: the first argument is DX-specific (maybe the entire function is)
    virtual bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned numElements) = 0;

    /*!
     * \brief Creates the buffer that will be used to send "input" (as per the spec) data to the shader
     * \return False if creating the input buffer failed, true otherwise.
     */
    virtual bool CreateInputBuffer() = 0;

    /*!
     * \brief Called before rendering.
     *        Updates any internal state needed to ensure that correct data is passed to the shader
     *        when rendering
     * \param dest Coordinates of the 4 corners of the output viewport/window
     * \param isLastPass True if the current shader is last one in the pipeline // TODO: this could be a member
     * \param frameCount Number of frames that have passed
     */
    virtual void PrepareParameters(CPoint dest[4], bool isLastPass, uint64_t frameCount) = 0;

    /*!
     * \brief Updates the model view projection matrix.
     *        Should usually only be called when the viewport/window size changes
     */
    virtual void UpdateMVP() = 0;

    virtual ~IVideoShader() = default;
  };
}
}
