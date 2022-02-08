/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRandomGenerator.h"

namespace KODI
{
namespace CRYPTO
{
class COpenSSLRandomGenerator : public IRandomGenerator
{
public:
  ~COpenSSLRandomGenerator() override;

  // Implementation of IRandomGenerator
  std::vector<uint8_t> RandomBytes(size_t length) override;
};
} // namespace CRYPTO
} // namespace KODI
