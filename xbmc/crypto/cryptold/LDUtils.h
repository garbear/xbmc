/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the crypto-ld project under the BSD
 *  3-Clause License - https://github.com/digitalbazaar/crypto-ld
 *  Copyright (C) 2018-2021 Digital Bazaar, Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

#include <string>

namespace KODI
{
namespace CRYPTO
{

class CLDUtils
{
public:
  /*!
   * \brief Finds a verification method for a given purpose (verification
   *        relationship)
   *
   * Returns the first available verification method for that purpose.
   *
   * \param purpose The verification relationship
   *
   * \return The method for the given purpose, or an empty CVariant if no
   *         method is found
   */
  static CVariant FindVerificationMethod(const CVariant& didDocument, const std::string& purpose);

private:
  /*!
   * \brief Finds a verification method for a given method ID and returns it
   *
   * \param didDocument The DID Document
   * \param methodId The verification method ID
   *
   * \return The verification method
   */
  static CVariant GetMethodByID(const CVariant& didDocument, const std::string& methodId);
};

} // namespace CRYPTO
} // namespace KODI
