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
#pragma once

#include <string>
#include <vector>
#include <map>
#include "ServiceBroker.h"
#include "utils/Geometry.h"

namespace KODI
{
namespace SHADER
{
  class IShaderTexture;

  enum FILTER_TYPE
  {
    FILTER_TYPE_NONE,
    FILTER_TYPE_LINEAR,
    FILTER_TYPE_NEAREST
  };

  enum WRAP_TYPE
  {
    WRAP_TYPE_BORDER,
    WRAP_TYPE_EDGE,
    WRAP_TYPE_REPEAT,
    WRAP_TYPE_MIRRORED_REPEAT,
  };

  enum SCALE_TYPE
  {
    SCALE_TYPE_INPUT,
    SCALE_TYPE_ABSOLUTE,
    SCALE_TYPE_VIEWPORT,
  };

  struct FboScaleAxis
  {
    SCALE_TYPE type = SCALE_TYPE_INPUT;
    float scale = 1.0;
    unsigned int abs = 1;
  };

  struct FboScale
  {
    bool sRgbFramebuffer = false;
    bool floatFramebuffer = false;
    FboScaleAxis scaleX;
    FboScaleAxis scaleY;
  };

  struct VideoShaderLut
  {
    std::string strId;
    std::string path;
    FILTER_TYPE filter = FILTER_TYPE_NONE;
    WRAP_TYPE wrap = WRAP_TYPE_BORDER;
    bool mipmap = false;
  };

  struct VideoShaderParameter
  {
    std::string strId;
    std::string description;
    float current = 0.0f;
    float minimum = 0.0f;
    float initial = 0.0f;
    float maximum = 0.0f;
    float step = 0.0f;
  };

  struct VideoShaderPass
  {
    std::string sourcePath;
    std::string vertexSource;
    std::string fragmentSource;
    FILTER_TYPE filter = FILTER_TYPE_NONE;
    WRAP_TYPE wrap = WRAP_TYPE_BORDER;
    unsigned int frameCountMod = 0;
    FboScale fbo;
    bool mipmap = false;

    std::vector<VideoShaderLut> luts;
    std::vector<VideoShaderParameter> parameters;
  };

  using ShaderParameters = std::map<std::string, float>;

  class IVideoShaderPreset
  {
  public:
    using VideoShaderPasses = std::vector<VideoShaderPass>;

    virtual ~IVideoShaderPreset() = default;

    //todo: impl once and for all
   /*!
    * \brief Reads/Parses a shader preset file and loads its state to the
    *        object. What this state is is implementation specific.
    * \param presetPath Full path of the preset file.
    * \return True on successful parsing, false on failed.
    */
    virtual bool ReadPresetFile(const std::string &presetPath) = 0;

    /*!
     * \brief Updates state if needed and renderes the preset to the target texture
     * \param dest Coordinates of the 4 corners of the output viewport/window
     * \param source Input texture. The source of the video frame, in is original resolution (unscaled)
     * \param target The target texture. The texture that the final result will be rendered in.
     * \return Returns false if updating or rendering failed, true if both succeeded.
     */
    virtual bool RenderUpdate(const CPoint dest[], IShaderTexture* source, IShaderTexture* target) = 0;

    /*!
     * \brief Informs about the speed of playback
     * \param speed Commonly 1.0 for normal playback, and 0.0 if paused
     */
    virtual void SetSpeed(double speed) = 0;

    /*!
     * \brief Size of the input/source frame in pixels
     * \param videoWidth Height of the source frame in pixels
     * \param videoHeight Height of the source frame in pixels
     */
    virtual void SetVideoSize(const unsigned videoWidth, const unsigned videoHeight) = 0;

    /*!
     * \brief Set the preset to be rendered on the next frame
     * \param shaderPresetPath Full path to the preset file to be loaded
     * \return Returns false if loading the preset failed, true otherwise.
     */
    virtual bool SetShaderPreset(const std::string& shaderPresetPath) = 0;

    /*!
     * \brief Gets the full path to the shader preset
     * \return The full path to the currently loaded preset file
     */
    virtual const std::string& GetShaderPreset() const = 0;

    /*!
     * \brief Gets the passes of the loaded preset
     * \return All the video shader passes of the currently loaded preset
     */
    virtual VideoShaderPasses& GetPasses() = 0;
  };
}
}
