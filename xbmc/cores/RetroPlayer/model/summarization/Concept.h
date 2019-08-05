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
  class CInstance;
  class CUnits;

  class CConcept
  {
  public:
    virtual ~CConcept() = default;

    std::shared_ptr<CInstance> instance;
    std::string symbol;
    std::shared_ptr<CUnits> units;
  };

  class CDictionaryConcept : public CConcept
  {
  public:
    CDictionaryConcept() = default;
    ~CDictionaryConcept() override = default;
  };

  class CValueConcept : public CConcept
  {
  public:
    CValueConcept() = default;
    ~CValueConcept() override = default;
  };
}
}
