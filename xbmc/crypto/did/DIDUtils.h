/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the js-did-ipid project under the
 *  MIT License - https://github.com/ipfs-shipyard/js-did-ipid
 *  Copyright (C) 2019 Protocol Labs Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND MIT
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/CryptoTypes.h"
#include "crypto/Key.h"

#include <string>

class CVariant;

namespace KODI
{
namespace CRYPTO
{

class CDIDUtils
{
public:
  static const std::string DEFAULT_CONTEXT;
  static const std::string SEPARATOR_PUBLIC_KEY;
  static const std::string SEPARATOR_SERVICE;

  /*!
   * \brief Returns the DID associated to a private key in PEM format
   *
   * Supported formats: pkcs1-pem or pkcs8-pem.
   *
   * \param pem A private key in PEM format
   *
   * \return The DID
   */
  static std::string GetDID(const PrivateKey& privateKey);

  static bool ParseDID(const std::string& did, std::string& method, std::string& identifier);

  static std::string CreatePublicKeyID(const std::string& did,
                                       std::string fragment,
                                       const std::string& prefix);

  static std::string CreateServiceID(const std::string& did,
                                     std::string fragment,
                                     const std::string& prefix);

  static bool IsEquivalentID(std::string id1, std::string id2, const std::string& separator);

  static std::string ParseAuthenticationID(const CVariant& authentication);

  static std::string GenerateRandomString();

  static std::string GenerateIpnsName(const PrivateKey& key);

private:
  static std::string GenerateDID(const PrivateKey& key);

  static std::string CreateID(const std::string& did,
                              std::string fragment,
                              const std::string& separator,
                              const std::string& prefix);
};
} // namespace CRYPTO
} // namespace KODI
