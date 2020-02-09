/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SummaryTypes.h"
#include "Variable.h"

#include <vector>

namespace KODI
{
  namespace RETRO
  {
    struct Expression
    {
      virtual ~Expression() = default;
    };

    struct VariableExpression : public Expression
    {
      Variable Variable;
    };

    enum class PolyadicOperator
    {
      ADDITION,
      MULTIPLICATION,
      MAXIMUM,
    };

    /*
    enum class TermType
    {

    }
    struct Term
    {
      Variable variable;
      bool invertbit; // Valid for single bits
    };
    */

    struct PolyadicExpression : Expression
    {
      PolyadicOperator op;
      std::vector<ExpressionPtr> operands;
    };
  }
}
