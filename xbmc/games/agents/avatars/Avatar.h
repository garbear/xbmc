/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include <string>

class TiXmlElement;

namespace KODI
{
namespace GAME
{

class CAvatar
{
public:
  CAvatar();

  const std::string& Author() const { return m_author; }

  const std::string& Source() const { return m_source; }

  const std::string& License() const { return m_license; }

  bool Deserialize(const TiXmlElement* pElement);

private:
  std::string m_author;
  std::string m_source;
  std::string m_license;
};

} // namespace GAME
} // namespace KODI
