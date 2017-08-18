/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderSamplerDX.h"
#include "system.h"

using namespace KODI;
using namespace SHADER;

CShaderSamplerDX::CShaderSamplerDX(ID3D11SamplerState* sampler)
  : m_sampler(sampler)
{
}

CShaderSamplerDX::~CShaderSamplerDX()
{
  SAFE_RELEASE(m_sampler);
}
