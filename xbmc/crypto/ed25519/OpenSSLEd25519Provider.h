/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IEd25519Provider.h"

namespace KODI
{
namespace CRYPTO
{
class COpenSSLEd25519Provider : public IEd25519Provider
{
public:
  ~COpenSSLEd25519Provider() override;

  // Implementation of IEd25519Provider
  KeyPair Generate() override;
};
} // namespace CRYPTO
} // namespace KODI
