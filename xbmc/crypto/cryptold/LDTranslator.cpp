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

#include "LDTranslator.h"

#include "crypto/ed25519/Ed25519VerificationKey2020.h"

using namespace KODI;
using namespace CRYPTO;

std::string CLDTranslator::TranslateKeyType(LDKeyType keyType)
{
  switch (keyType)
  {
    case LDKeyType::Ed25519VerificationKey2020:
      return "Ed25519VerificationKey2020";
    default:
      break;
  }

  return "";
}

LDKeyType CLDTranslator::TranslateKeyType(const std::string& keyType)
{
  if (keyType == "Ed25519VerificationKey2020")
    return LDKeyType::Ed25519VerificationKey2020;

  return LDKeyType::UNKNOWN;
}

std::string CLDTranslator::GetLDContextBySuite(LDKeyType keyType)
{
  switch (keyType)
  {
    case LDKeyType::Ed25519VerificationKey2020:
      return CEd25519VerificationKey2020::SUITE_CONTEXT;
    default:
      break;
  }

  return "";
}
