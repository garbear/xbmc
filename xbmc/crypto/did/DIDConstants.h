/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the did-io project under the BSD
 *  3-Clause License - https://github.com/digitalbazaar/did-io
 *  Copyright (C) 2018-2021 Digital Bazaar, Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <string>

namespace KODI
{
namespace CRYPTO
{

class CDIDConstants
{
public:
  /*!
   * \brief Container of valid verification relationships in a DID Document
   *
   * A verification relationship expresses the relationship between the DID
   * subject and a verification method (such as a cryptographic public key).
   * Different verification relationships enable the associated verification
   * methods to be used for different purposes.
   *
   * See:
   *
   *   - https://w3c.github.io/did-core/#verification-relationships
   */
  static const std::array<std::string, 5> VERIFICATION_RELATIONSHIPS;
};

} // namespace CRYPTO
} // namespace KODI
