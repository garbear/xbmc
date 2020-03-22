/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRandomGenerator.h"

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace KODI
{
namespace CRYPTO
{
  class CBoostRandomGenerator : public IRandomGenerator
  {
  public:
    ~CBoostRandomGenerator() override;

    // Implementation of IRandomGenerator
    std::vector<uint8_t> RandomBytes(size_t length) override;

  private:
    // Boost cryptographic-secure random generator
    boost::random_device m_generator;

    // Uniform distribution tool
    boost::random::uniform_int_distribution<uint8_t> m_distribution;
  };
}
}
