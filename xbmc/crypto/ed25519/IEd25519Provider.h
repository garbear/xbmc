/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/Key.h"

namespace KODI
{
namespace CRYPTO
{
class IEd25519Provider
{
public:
  virtual ~IEd25519Provider() = default;

  /*!
   * \brief Generate key pair
   *
   * \return The key pair
   */
  virtual KeyPair Generate() = 0;
};
} // namespace CRYPTO
} // namespace KODI
