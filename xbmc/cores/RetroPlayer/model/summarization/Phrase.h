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
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CCondition;
  class CLanguageString;
  class CMolecule;

  class CPhrase
  {
  public:
    CPhrase() = default;

    std::shared_ptr<CCondition> condition;
    std::vector<std::shared_ptr<CMolecule>> molecules;
    std::shared_ptr<CLanguageString> phraseTemplate;
  };
}
}
