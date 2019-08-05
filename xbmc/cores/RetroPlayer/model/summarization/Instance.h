/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

namespace KODI
{
namespace RETRO
{
  class CLanguageString;

  class CInstance
  {
  public:
    virtual ~CInstance() = 0;

    std::shared_ptr<CLanguageString> label;
    std::string value;
  };
}
}
