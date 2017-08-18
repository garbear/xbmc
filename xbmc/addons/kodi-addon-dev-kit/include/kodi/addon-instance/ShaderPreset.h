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

#include "../AddonBase.h"

#include <stdint.h>

namespace kodi { namespace addon { class CInstanceShaderPreset; } }

extern "C"
{
  typedef void *preset_file;

  /*!
   * \brief Scale types
   *
   * If no scale type is specified, it is assumed that the scale type is
   * relative to the input with a scaling factor of 1.0.
   *
   * Exceptions: If no scale type is set for the last pass, it is assumed to
   * output at the full resolution rather than assuming of scale of 1.0, and
   * bypasses any frame-buffer object rendering.
   */
  typedef enum SHADER_SCALE_TYPE
  {
    /*!
     * \brief Use the source size
     *
     * Output size of the shader pass is relative to the input size. Value is
     * float.
     */
    SHADER_SCALE_TYPE_INPUT,

    /*!
     * \brief Use the window viewport size
     *
     * Output size of the shader pass is relative to the size of the window
     * viewport. Value is float. This value can change over time if the user
     * resizes his/her window!
     */
    SHADER_SCALE_TYPE_ABSOLUTE,

    /*!
     * \brief Use a statically defined size
     *
     * Output size is statically defined to a certain size. Useful for hi-res
     * blenders or similar.
     */
    SHADER_SCALE_TYPE_VIEWPORT
  } SHADER_SCALE_TYPE;

  typedef enum SHADER_FILTER_TYPE
  {
    SHADER_FILTER_TYPE_UNSPEC,
    SHADER_FILTER_TYPE_LINEAR,
    SHADER_FILTER_TYPE_NEAREST
  } SHADER_FILTER_TYPE;

  /*!
   * \brief Texture wrapping mode
   */
  typedef enum SHADER_WRAP_TYPE
  {
    SHADER_WRAP_TYPE_BORDER, /* Deprecated, will be translated to EDGE in GLES */
    SHADER_WRAP_TYPE_EDGE,
    SHADER_WRAP_TYPE_REPEAT,
    SHADER_WRAP_TYPE_MIRRORED_REPEAT
  } SHADER_WRAP_TYPE;

  /*!
   * \brief FBO scaling parameters for a single axis
   */
  typedef struct fbo_scale_axis
  {
    SHADER_SCALE_TYPE type;
    union
    {
      float scale;
      unsigned abs;
    };
  } fbo_scale_axis;

  /*!
   * \brief FBO parameters
   */
  typedef struct fbo_scale
  {
    /*!
     * \brief sRGB framebuffer
     */
    bool srgb_fbo;

    /*!
     * \brief Float framebuffer
     *
     * This parameter defines if the pass should be rendered to a 32-bit
     * floating point buffer. This only takes effect if the pass is actaully
     * rendered to an FBO. This is useful for shaders which have to store FBO
     * values outside the range [0, 1].
     */
    bool fp_fbo;

    /*!
     * \brief Scaling parameters for X axis
     */
    fbo_scale_axis scale_x;

    /*!
     * \brief Scaling parameters for Y axis
     */
    fbo_scale_axis scale_y;
  } fbo_scale;

  typedef struct video_shader_parameter
  {
    char *id;
    char *desc;
    float current;
    float minimum;
    float initial;
    float maximum;
    float step;
  } video_shader_parameter;

  typedef struct video_shader_pass
  {
    /*!
     * \brief Path to the shader pass source
     */
    char *source_path;

    /*!
     * \brief The vertex shader source
     */
    char *vertex_source;

    /*!
     * \brief The fragment shader source, if separate from the vertex source, or
     * NULL otherwise
     */
    char *fragment_source;

    /*!
     * \brief FBO parameters
     */
    fbo_scale fbo;

    /*!
     * \brief Defines how the result of this pass will be filtered
     *
     * @todo Define behavior for unspecified filter
     */
    SHADER_FILTER_TYPE filter;

    /*!
     * \brief Wrapping mode
     */
    SHADER_WRAP_TYPE wrap;

    /*!
     * \brief Frame count mod
     */
    unsigned frame_count_mod;

    /*!
     * \brief Mipmapping
     */
    bool mipmap;
  } video_shader_pass;

  typedef struct video_shader_lut
  {
    /*!
     * \brief Name of the sampler uniform, e.g. `uniform sampler2D foo`.
     */
    char *id;

    /*!
     * \brief Path of the texture
     */
    char *path;

    /*!
     * \brief Filtering for the texture
     */
    SHADER_FILTER_TYPE filter;

    /*!
     * \brief Texture wrapping mode
     */
    SHADER_WRAP_TYPE wrap;

    /*!
     * \brief Use mipmapping for the texture
     */
    bool mipmap;
  } video_shader_lut;

  typedef struct video_shader
  {
    unsigned pass_count;
    video_shader_pass *passes;

    unsigned lut_count;
    video_shader_lut *luts;

    unsigned parameter_count;
    video_shader_parameter *parameters;
  } video_shader;
  ///}

  typedef struct AddonProps_ShaderPreset
  {
    const char* user_path;              /*!< @brief path to the user profile */
    const char* addon_path;             /*!< @brief path to this add-on */
  } AddonProps_ShaderPreset;

  struct AddonInstance_ShaderPreset;

  typedef struct AddonToKodiFuncTable_ShaderPreset
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_ShaderPreset;

  typedef struct KodiToAddonFuncTable_ShaderPreset
  {
    kodi::addon::CInstanceShaderPreset* addonInstance;

    preset_file (__cdecl* preset_file_new)(const AddonInstance_ShaderPreset* addonInstance, const char *path);
    void (__cdecl* preset_file_free)(const AddonInstance_ShaderPreset* addonInstance, preset_file file);

    bool (__cdecl* video_shader_read)(const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader *shader);
    void (__cdecl* video_shader_write)(const AddonInstance_ShaderPreset* addonInstance, preset_file file, const video_shader *shader);
    //void (__cdecl* video_shader_resolve_relative)(const AddonInstance_ShaderPreset* addonInstance, video_shader *shader, const char *ref_path);
    //bool (__cdecl* video_shader_resolve_current_parameters)(const AddonInstance_ShaderPreset* addonInstance, video_shader *shader);
    bool (__cdecl* video_shader_resolve_parameters)(const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader *shader);
    void (__cdecl* video_shader_free)(const AddonInstance_ShaderPreset* addonInstance, video_shader *shader);
  } KodiToAddonFuncTable_ShaderPreset;

  typedef struct AddonInstance_ShaderPreset
  {
    AddonProps_ShaderPreset props;
    AddonToKodiFuncTable_ShaderPreset toKodi;
    KodiToAddonFuncTable_ShaderPreset toAddon;
  } AddonInstance_ShaderPreset;

} /* extern "C" */

namespace kodi
{
  namespace addon
  {

    class CInstanceShaderPreset : public IAddonInstance
    {
    public:
      CInstanceShaderPreset()
        : IAddonInstance(ADDON_INSTANCE_SHADERPRESET)
      {
        if (CAddonBase::m_interface->globalSingleInstance != nullptr)
          throw std::logic_error("kodi::addon::CInstanceShaderPreset: Creation of more as one in single instance way is not allowed!");

        SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
        CAddonBase::m_interface->globalSingleInstance = this;
      }

      CInstanceShaderPreset(KODI_HANDLE instance)
        : IAddonInstance(ADDON_INSTANCE_SHADERPRESET)
      {
        if (CAddonBase::m_interface->globalSingleInstance != nullptr)
          throw std::logic_error("kodi::addon::CInstanceShaderPreset: Creation of multiple together with single instance way is not allowed!");

        SetAddonStruct(instance);
      }

      ~CInstanceShaderPreset() override { }

      /*!
       * \brief Loads a preset file
       *
       * \param path The path to the preset file
       *
       * \return The preset file, or NULL if file doesn't exist
       */
      virtual preset_file PresetFileNew(const char *path) { return nullptr; }

      /*!
       * \brief Free a preset file
       */
      virtual void PresetFileFree(preset_file file) { }

      /*!
       * \brief Loads preset file and all associated state (passes, textures,
       * imports, etc)
       *
       * \param file              Preset file to read from
       * \param shader            Shader passes handle
       *
       * \return True if successful, otherwise false
       **/
      virtual bool ShaderPresetRead(preset_file file, video_shader &shader) { return false; }

      /*!
       * \brief Save preset and all associated state (passes, textures, imports,
       * etc) to disk
       *
       * \param file              Preset file to read from
       * \param shader            Shader passes handle
       */
      virtual void ShaderPresetWrite(preset_file file, const video_shader &shader) { }

      /*!
       * \brief Resolve relative shader path (@ref_path) into absolute shader path
       *
       * \param shader            Shader pass handle
       * \param ref_path          Relative shader path
       */
      //virtual void ShaderPresetResolveRelative(video_shader &shader, const char *ref_path) { }

      /*!
       * \brief Read the current value for all parameters from preset file
       *
       * \param file              Preset file to read from
       * \param shader            Shader passes handle
       *
       * \return True if successful, otherwise false
       */
      //virtual bool ShaderPresetResolveCurrentParameters(preset_file file, video_shader &shader) { return false; }

      /*!
       * \brief Resolve all shader parameters belonging to the shader preset
       *
       * \param file              Preset file to read from
       * \param shader            Shader passes handle
       *
       * \return True if successful, otherwise false
       */
      virtual bool ShaderPresetResolveParameters(preset_file file, video_shader &shader) { return false; }

      /*!
       * \brief Free all state related to shader preset
       *
       * \param shader Object to free
       */
      virtual void ShaderPresetFree(video_shader &shader) { }

      std::string AddonPath() const
      {
        if (m_instanceData->props.addon_path)
          return m_instanceData->props.addon_path;
        return "";
      }

      std::string UserPath() const
      {
        if (m_instanceData->props.user_path)
          return m_instanceData->props.user_path;
        return "";
      }

    private:
      void SetAddonStruct(KODI_HANDLE instance)
      {
        if (instance == nullptr)
          throw std::logic_error("kodi::addon::CInstanceShaderPreset: Creation with empty addon structure not allowed, table must be given from Kodi!");

        m_instanceData = static_cast<AddonInstance_ShaderPreset*>(instance);
        m_instanceData->toAddon.addonInstance = this;

        m_instanceData->toAddon.preset_file_new = ADDON_preset_file_new;
        m_instanceData->toAddon.preset_file_free = ADDON_preset_file_free;

        m_instanceData->toAddon.video_shader_read = ADDON_video_shader_read_file;
        m_instanceData->toAddon.video_shader_write = ADDON_video_shader_write_file;
        //m_instanceData->toAddon.video_shader_resolve_relative = ADDON_video_shader_resolve_relative;
        //m_instanceData->toAddon.video_shader_resolve_current_parameters = ADDON_video_shader_resolve_current_parameters;
        m_instanceData->toAddon.video_shader_resolve_parameters = ADDON_video_shader_resolve_parameters;
        m_instanceData->toAddon.video_shader_free = ADDON_video_shader_free;
      }

      inline static preset_file ADDON_preset_file_new(const AddonInstance_ShaderPreset* addonInstance, const char *path)
      {
        return addonInstance->toAddon.addonInstance->PresetFileNew(path);
      }

      inline static void ADDON_preset_file_free(const AddonInstance_ShaderPreset* addonInstance, preset_file file)
      {
        return addonInstance->toAddon.addonInstance->PresetFileFree(file);
      }

      inline static bool ADDON_video_shader_read_file(const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader *shader)
      {
        if (shader != nullptr)
          return addonInstance->toAddon.addonInstance->ShaderPresetRead(file, *shader);

        return false;
      }

      inline static void ADDON_video_shader_write_file(const AddonInstance_ShaderPreset* addonInstance, preset_file file, const video_shader *shader)
      {
        if (shader != nullptr)
          addonInstance->toAddon.addonInstance->ShaderPresetWrite(file, *shader);
      }

      /*
      inline static void ADDON_video_shader_resolve_relative(const AddonInstance_ShaderPreset* addonInstance, video_shader *shader, const char *ref_path)
      {
        if (shader != nullptr)
          addonInstance->toAddon.addonInstance->ShaderPresetResolveRelative(*shader, ref_path);
      }

      inline static bool ADDON_video_shader_resolve_current_parameters(const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader *shader)
      {
        if (shader != nullptr)
          return addonInstance->toAddon.addonInstance->ShaderPresetResolveCurrentParameters(file, *shader);

        return false;
      }
      */

      inline static bool ADDON_video_shader_resolve_parameters(const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader *shader)
      {
        if (shader != nullptr)
          return addonInstance->toAddon.addonInstance->ShaderPresetResolveParameters(file, *shader);
        
        return false;
      }

      inline static void ADDON_video_shader_free(const AddonInstance_ShaderPreset* addonInstance, video_shader *shader)
      {
        if (shader != nullptr)
          addonInstance->toAddon.addonInstance->ShaderPresetFree(*shader);
      }

      AddonInstance_ShaderPreset* m_instanceData;
    };

  } /* namespace addon */
} /* namespace kodi */
