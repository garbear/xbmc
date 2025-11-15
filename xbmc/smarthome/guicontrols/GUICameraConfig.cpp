/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUICameraConfig.h"

#include "smarthome/guicontrols/GUICameraControl.h"
#include "utils/Geometry.h" //! @todo For IDE

using namespace KODI;
using namespace SMART_HOME;

namespace
{
constexpr const char* DEFAULT_IMAGE_TRANSPORT = "compressed";
} // namespace

CGUICameraConfig::CGUICameraConfig() : m_topic(), m_imageTransport(DEFAULT_IMAGE_TRANSPORT)
{
}

CGUICameraConfig::CGUICameraConfig(const CGUICameraConfig& other)
  : m_topic(other.m_topic),
    m_imageTransport(other.m_imageTransport)
{
}

CGUICameraConfig::~CGUICameraConfig() = default;

void CGUICameraConfig::Reset()
{
  m_topic.clear();
  m_imageTransport = DEFAULT_IMAGE_TRANSPORT;
}
