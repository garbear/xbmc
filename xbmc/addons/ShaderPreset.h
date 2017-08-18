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

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/ShaderPreset.h"
#include "cores/RetroPlayer/rendering/VideoShaders/VideoShaderPresetFactory.h"
#include "threads/SharedSection.h"

#include <string>
#include <vector>

namespace ADDON
{
  class CShaderPreset
  {
  public:
    CShaderPreset(preset_file file, AddonInstance_ShaderPreset &instanceStruct);
    ~CShaderPreset();

    /*!
     * \brief @todo document
     */
    bool ReadShaderPreset(video_shader &shader);

    /*!
     * \brief @todo document
     */
    void WriteShaderPreset(const video_shader &shader);

    //void ResolveRelative(video_shader &shader, const std::string &ref_path);

    //bool ResolveCurrentParameters(video_shader &shader);

    /*!
     * \brief @todo document
     */
    bool ResolveParameters(video_shader &shader);

    void FreeShaderPreset(video_shader &shader);

  private:
    preset_file m_file;
    AddonInstance_ShaderPreset &m_struct;
  };

  /*!
   * \brief Wrapper class that wraps the shader presets add-on
   */
  class CShaderPresetAddon : public IAddonInstanceHandler,
                             public KODI::SHADER::IVideoShaderPresetLoader
  {
  public:
    CShaderPresetAddon(const BinaryAddonBasePtr& addonInfo);
    ~CShaderPresetAddon(void) override;

    /*!
     * \brief Initialise the instance of this add-on
     */
    bool CreateAddon(void);

    /*!
     * \brief Deinitialize the instance of this add-on
     */
    void DestroyAddon();

    /*!
     * \brief Get the shader preset extensions supported by this add-on
     */
    const std::vector<std::string> &GetExtensions() const { return m_extensions; }

    // implementation of IVideoShaderPresetLoader
    bool LoadPreset(const std::string &presetPath, KODI::SHADER::IVideoShaderPreset &shaderPreset) override;

  private:
    /*!
     * \brief Reset all class members to their defaults. Called by the constructors
     */
    void ResetProperties(void);

    static void TranslateShaderPreset(const video_shader &shader, KODI::SHADER::IVideoShaderPreset& shaderPreset);
    static void TranslateShaderPass(const video_shader_pass &pass, KODI::SHADER::VideoShaderPass &shaderPass);
    static void TranslateShaderLut(const video_shader_lut &lut, KODI::SHADER::VideoShaderLut &shaderLut);
    static void TranslateShaderParameter(const video_shader_parameter &param, KODI::SHADER::VideoShaderParameter &shaderParam);
    static KODI::SHADER::FILTER_TYPE TranslateFilterType(SHADER_FILTER_TYPE type);
    static KODI::SHADER::WRAP_TYPE TranslateWrapType(SHADER_WRAP_TYPE type);
    static KODI::SHADER::SCALE_TYPE TranslateScaleType(SHADER_SCALE_TYPE type);

    /* \brief Cache for const char* members in PERIPHERAL_PROPERTIES */

    std::string         m_strLibraryPath;
    std::string         m_strConfigPath;
    std::string         m_strConfigBasePath;
    std::string         m_strUserPath;    /*!< @brief translated path to the user profile */
    std::string         m_strClientPath;  /*!< @brief translated path to this add-on */

    /* \brief Add-on properties */
    std::vector<std::string> m_extensions;

    /* \brief Thread synchronization */
    CCriticalSection    m_critSection;

    AddonInstance_ShaderPreset m_struct;

    CSharedSection      m_dllSection;
  };
}
