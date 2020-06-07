/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Cryptographically secure pseudorandom number generator (CSPRNG)
 */
class IRandomGenerator
{
public:
  virtual ~IRandomGenerator() = default;

  /*!
   * \brief Generate random bytes
   *
   * \param length The number of types
   *
   * \return Buffer containing random bytes
   */
  virtual std::vector<uint8_t> RandomBytes(size_t length) = 0;
};
} // namespace CRYPTO
} // namespace KODI
