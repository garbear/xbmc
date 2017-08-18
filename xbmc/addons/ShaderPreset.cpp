/*
 *      Copyright (C) 2017 Team Kodi
 *     http://kodi.tv
 *
 * This Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this Program; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include "ShaderPreset.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/AddonManager.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace ADDON;

// --- CShaderPreset -----------------------------------------------------------

CShaderPreset::CShaderPreset(preset_file file, AddonInstance_ShaderPreset &instanceStruct) :
  m_file(file),
  m_struct(instanceStruct)
{
}

CShaderPreset::~CShaderPreset()
{
  m_struct.toAddon.preset_file_free(&m_struct, m_file);
}

bool CShaderPreset::ReadShaderPreset(video_shader &shader)
{
  return m_struct.toAddon.video_shader_read(&m_struct, m_file, &shader);
}

void CShaderPreset::WriteShaderPreset(const video_shader &shader)
{
  return m_struct.toAddon.video_shader_write(&m_struct, m_file, &shader);
}

/*
void CShaderPresetAddon::ShaderPresetResolveRelative(video_shader &shader, const std::string &ref_path)
{
  return m_struct.toAddon.video_shader_resolve_relative(&m_struct, &shader, ref_path.c_str());
}

bool CShaderPresetAddon::ShaderPresetResolveCurrentParameters(video_shader &shader)
{
  return m_struct.toAddon.video_shader_resolve_current_parameters(&m_struct, m_file, &shader);
}
*/

bool CShaderPreset::ResolveParameters(video_shader &shader)
{
  return m_struct.toAddon.video_shader_resolve_parameters(&m_struct, m_file, &shader);
}

void CShaderPreset::FreeShaderPreset(video_shader &shader)
{
  m_struct.toAddon.video_shader_free(&m_struct, &shader);
}

// --- CShaderPresetAddon ------------------------------------------------------

CShaderPresetAddon::CShaderPresetAddon(const BinaryAddonBasePtr& addonBase)
  : IAddonInstanceHandler(ADDON_INSTANCE_SHADERPRESET, addonBase)
{
  ResetProperties();
  m_extensions = StringUtils::Split(addonBase->Type(ADDON_SHADERDLL)->GetValue("@extensions").asString(), "|");
}

CShaderPresetAddon::~CShaderPresetAddon(void)
{
  DestroyAddon();
}

bool CShaderPresetAddon::CreateAddon(void)
{
  CExclusiveLock lock(m_dllSection);

  // Reset all properties to defaults
  ResetProperties();

  // Initialise the add-on
  CLog::Log(LOGDEBUG, "%s - creating ShaderPreset add-on instance '%s'", __FUNCTION__, Name().c_str());

  if (CreateInstance(&m_struct) != ADDON_STATUS_OK)
    return false;

  return true;
}

void CShaderPresetAddon::DestroyAddon()
{
  CExclusiveLock lock(m_dllSection);
  DestroyInstance();
}

void CShaderPresetAddon::ResetProperties(void)
{
  // Initialise members
  m_struct = {{ 0 }};
  m_struct.toKodi.kodiInstance = this;
}

bool CShaderPresetAddon::LoadPreset(const std::string &presetPath, SHADER::IVideoShaderPreset& shaderPreset)
{
  bool bSuccess = false;

  std::string translatedPath = CSpecialProtocol::TranslatePath(presetPath);

  preset_file file = m_struct.toAddon.preset_file_new(&m_struct, translatedPath.c_str());

  if (file != nullptr)
  {
    std::unique_ptr<CShaderPreset> shaderPresetAddon(new CShaderPreset(file, m_struct));

    video_shader videoShader = { };
    if (shaderPresetAddon->ReadShaderPreset(videoShader))
    {
      if (shaderPresetAddon->ResolveParameters(videoShader))
      {
        TranslateShaderPreset(videoShader, shaderPreset);
        bSuccess = true;
      }
      shaderPresetAddon->FreeShaderPreset(videoShader);
    }
  }

  return bSuccess;
}

// todo: instead of copying every parameter to every pass and resolving them later in
//       GetShaderParameters, we should resolve which param goes to which shader in the add-on
void CShaderPresetAddon::TranslateShaderPreset(const video_shader &shader, SHADER::IVideoShaderPreset &shaderPreset)
{
  if (shader.passes != nullptr)
  {
    for (unsigned int passIdx = 0; passIdx < shader.pass_count; passIdx++)
    {
      SHADER::VideoShaderPass shaderPass;
      TranslateShaderPass(shader.passes[passIdx], shaderPass);

      if (shader.luts != nullptr)
      {
        for (unsigned int lutIdx = 0; lutIdx < shader.lut_count; lutIdx++)
        {
          SHADER::VideoShaderLut shaderLut;
          TranslateShaderLut(shader.luts[lutIdx], shaderLut);
          shaderPass.luts.emplace_back(std::move(shaderLut));
        }
      }

      if (shader.parameters != nullptr)
      {
        for (unsigned int parIdx = 0; parIdx < shader.parameter_count; parIdx++)
        {
          SHADER::VideoShaderParameter shaderParam;
          TranslateShaderParameter(shader.parameters[parIdx], shaderParam);
          shaderPass.parameters.emplace_back(std::move(shaderParam));
        }
      }

      shaderPreset.GetPasses().emplace_back(std::move(shaderPass));
    }
  }
}

void CShaderPresetAddon::TranslateShaderPass(const video_shader_pass &pass, SHADER::VideoShaderPass &shaderPass)
{
  shaderPass.sourcePath = pass.source_path ? pass.source_path : "";
  shaderPass.vertexSource = pass.vertex_source ? pass.vertex_source : "";
  shaderPass.fragmentSource = pass.fragment_source ? pass.fragment_source : "";
  shaderPass.filter = TranslateFilterType(pass.filter);
  shaderPass.wrap = TranslateWrapType(pass.wrap);
  shaderPass.frameCountMod = pass.frame_count_mod;

  const auto &fbo = pass.fbo;
  auto &shaderFbo = shaderPass.fbo;

  shaderFbo.scaleX.type = TranslateScaleType(fbo.scale_x.type);
  switch (fbo.scale_x.type)
  {
  case SHADER_SCALE_TYPE_ABSOLUTE:
    shaderFbo.scaleX.abs = fbo.scale_x.abs;
    break;
  default:
    shaderFbo.scaleX.scale = fbo.scale_x.scale;
    break;
  }
  shaderFbo.scaleY.type = TranslateScaleType(fbo.scale_y.type);
  switch (fbo.scale_y.type)
  {
  case SHADER_SCALE_TYPE_ABSOLUTE:
    shaderFbo.scaleY.abs = fbo.scale_y.abs;
    break;
  default:
    shaderFbo.scaleY.scale = fbo.scale_y.scale;
    break;
  }

  shaderFbo.floatFramebuffer = fbo.fp_fbo;
  shaderFbo.sRgbFramebuffer = fbo.srgb_fbo;

  shaderPass.mipmap = pass.mipmap;
}

void CShaderPresetAddon::TranslateShaderLut(const video_shader_lut &lut, SHADER::VideoShaderLut &shaderLut)
{
  shaderLut.strId = lut.id ? lut.id : "";
  shaderLut.path = lut.path ? lut.path : "";
  shaderLut.filter = TranslateFilterType(lut.filter);
  shaderLut.wrap = TranslateWrapType(lut.wrap);
  shaderLut.mipmap = lut.mipmap;
}

void CShaderPresetAddon::TranslateShaderParameter(const video_shader_parameter &param, SHADER::VideoShaderParameter &shaderParam)
{
  shaderParam.strId = param.id ? param.id : "";
  shaderParam.description = param.desc ? param.desc : "";
  shaderParam.current = param.current;
  shaderParam.minimum = param.minimum;
  shaderParam.initial = param.initial;
  shaderParam.maximum = param.maximum;
  shaderParam.step = param.step;
}

SHADER::FILTER_TYPE CShaderPresetAddon::TranslateFilterType(SHADER_FILTER_TYPE type)
{
  switch (type)
  {
  case SHADER_FILTER_TYPE_LINEAR:
    return SHADER::FILTER_TYPE_LINEAR;
  case SHADER_FILTER_TYPE_NEAREST:
    return SHADER::FILTER_TYPE_NEAREST;
  default:
    break;
  }

  return SHADER::FILTER_TYPE_NONE;
}

SHADER::WRAP_TYPE CShaderPresetAddon::TranslateWrapType(SHADER_WRAP_TYPE type)
{
  switch (type)
  {
  case SHADER_WRAP_TYPE_BORDER:
    return SHADER::WRAP_TYPE_BORDER;
  case SHADER_WRAP_TYPE_EDGE:
    return SHADER::WRAP_TYPE_EDGE;
  case SHADER_WRAP_TYPE_REPEAT:
    return SHADER::WRAP_TYPE_REPEAT;
  case SHADER_WRAP_TYPE_MIRRORED_REPEAT:
    return SHADER::WRAP_TYPE_MIRRORED_REPEAT;
  default:
    break;
  }

  return SHADER::WRAP_TYPE_BORDER;
}

SHADER::SCALE_TYPE CShaderPresetAddon::TranslateScaleType(SHADER_SCALE_TYPE type)
{
  switch (type)
  {
  case SHADER_SCALE_TYPE_INPUT:
    return SHADER::SCALE_TYPE_INPUT;
  case SHADER_SCALE_TYPE_ABSOLUTE:
    return SHADER::SCALE_TYPE_ABSOLUTE;
  case SHADER_SCALE_TYPE_VIEWPORT:
    return SHADER::SCALE_TYPE_VIEWPORT;
  default:
    break;
  }

  return SHADER::SCALE_TYPE_INPUT;
}
