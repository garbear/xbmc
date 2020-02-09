/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Molucule.h"
#include "SummaryTypes.h"

#include <vector>

namespace KODI
{
  namespace RETRO
  {
    class CTerm
    {
    public:
      virtual ~CTerm() = default;
      virtual std::string Instantiate() = 0;
    }

    class CPhrasePart : public CTerm
    {
      std::string phrasePart;
    };

    class C
    struct Term
    {
      std::string GetValue();
    }
    enum class TermType
    {
      STRING,
      MOLECULE,
    }

    struct TermString
    struct Phrase
    {
      std::vector<Variable> template_;
      ConditionPtr condition;
      std::vector<Molucule> molecules;
    };

    struct Phrase
    {
      Template template_;
      CVariant condition;
      std::vector<Molecule> molecules;
    };

  }
}
