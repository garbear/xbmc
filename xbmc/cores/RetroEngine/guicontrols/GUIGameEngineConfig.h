/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace RETRO_ENGINE
{
class CGUIGameEngineConfig
{
public:
  CGUIGameEngineConfig();
  CGUIGameEngineConfig(const CGUIGameEngineConfig& other);
  ~CGUIGameEngineConfig();

  void Reset();

  // Camera configuration
  const std::string& GetSavestate() const { return m_savestatePath; }
  void SetSavestate(const std::string& savestatePath) { m_savestatePath = savestatePath; }

private:
  std::string m_savestatePath;
};
} // namespace RETRO_ENGINE
} // namespace KODI
