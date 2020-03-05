/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI
{
  namespace RETRO
  {
    struct Concept;
    using ConceptPtr = std::unique_ptr<Concept>;

    struct Expression;
    using ExpressionPtr = std::unique_ptr<Expression>;
  }
}
