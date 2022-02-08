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

#include "LDTypes.h"

#include <string>

namespace KODI
{
namespace CRYPTO
{

class CLDTranslator
{
public:
  static std::string TranslateKeyType(LDKeyType keyType);
  static LDKeyType TranslateKeyType(const std::string& keyType);

  static std::string GetLDContextBySuite(LDKeyType keyType);
};

} // namespace CRYPTO
} // namespace KODI
