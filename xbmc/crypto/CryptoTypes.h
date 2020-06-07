/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Sequence of bytes
 */
using ByteArray = std::vector<uint8_t>;

/*!
 * \brief Hash256 as a sequence of 32 bytes
 */
using Hash256 = std::array<uint8_t, 32u>;

/*!
 * \brief Hash512 as a sequence of 64 bytes
 */
using Hash512 = std::array<uint8_t, 64u>;
} // namespace CRYPTO
} // namespace KODI
