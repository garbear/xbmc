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

#include "LDUtils.h"

#include "crypto/did/DIDConstants.h"

#include <algorithm>

using namespace KODI;
using namespace CRYPTO;

CVariant CLDUtils::FindVerificationMethod(const CVariant& didDocument, const std::string& purpose)
{
  if (didDocument.empty())
    return CVariant{}; //! @todo Throw 'A DID Document is required'

  if (purpose.empty())
    return CVariant{}; //! @todo Throw 'A purpose is required'

  // Find the first method by purpose
  CVariant method = didDocument[purpose][0];
  if (method.isString())
  {
    // This is a reference, not the full method, attempt to find it
    method = GetMethodByID(didDocument, method.asString());
  }

  return method;
}

CVariant CLDUtils::GetMethodByID(const CVariant& didDocument, const std::string& methodId)
{
  auto findByMethodId = [&methodId](const CVariant& method) {
    return method["id"].asString() == methodId;
  };

  // Search verification methods
  const CVariant& verificationMethods = didDocument["verificationMethod"];

  auto itMethod = std::find_if(verificationMethods.begin_array(), verificationMethods.end_array(),
                               findByMethodId);

  if (itMethod != verificationMethods.end_array())
    return *itMethod;

  // Search verification relationships
  for (const std::string& purpose : CDIDConstants::VERIFICATION_RELATIONSHIPS)
  {
    const CVariant& methods = didDocument[purpose];

    auto itMethod2 = std::find_if(methods.begin_array(), methods.end_array(), findByMethodId);

    if (itMethod2 != methods.end_array())
      return *itMethod2;
  }

  return CVariant{};
}
