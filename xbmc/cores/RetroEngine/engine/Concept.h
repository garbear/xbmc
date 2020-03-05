/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SummaryTypes.h"

#include <map>
#include <string>

namespace KODI
{
namespace RETRO
{
  struct Concept
  {
    virtual ~Concept() = default;
  };

  struct DictionaryConcept : public Concept
  {
    using Value = unsigned int;
    using Label = std::string;

    std::map<Value, Label> instances;
  };

  enum class UnitType
  {
    NONE,
  };

  struct ValueConcept : public Concept
  {
    UnitType units;
  };

  using ConceptSymbol = std::string;

  using ConceptMap = std::map<ConceptSymbol, ConceptPtr>;
}
}
