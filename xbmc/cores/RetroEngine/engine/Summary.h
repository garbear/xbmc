/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Concept.h"
#include "Molecule.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
  namespace RETRO
  {
    struct Instance
    {
      CVariant value;
      CVariant label;
    };

    struct Molecule
    {
      CVariant symbol;
      CVariant index;
      CVariant expression;
    };

    struct Summary
    {
      std::vector<Molecule> molecules;
      ConceptMap concepts;
    };

    class CSummaryEngine
    {
    public:
      Summary LoadSummary(const CVariant &summaryData);

      static std::string GetSummary(const Summary &summary, const uint8_t* state, size_t stateSize);

      std::string InstantiateUnits(const ConceptPtr &value, UnitType units);

    private:
      static std::string GetPhrase(const Template &template_, const std::vector<Concept> &concepts);
    };
  }
}
