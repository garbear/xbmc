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

#include "LDKeyPair.h"

#include "LDTranslator.h"

using namespace KODI;
using namespace CRYPTO;

CLDKeyPair::CLDKeyPair(std::string strId, std::string controller, std::string revoked)
  : m_id(std::move(strId)), m_controller(std::move(controller)), m_revoked(std::move(revoked))
{
}

CLDKeyPair::~CLDKeyPair() = default;

CVariant CLDKeyPair::Export(bool publicKey, bool privateKey, bool includeContext) const
{
  CVariant exportedKey{CVariant::VariantTypeObject};

  exportedKey["id"] = m_id;
  exportedKey["type"] = CLDTranslator::TranslateKeyType(GetType());

  if (!m_controller.empty())
    exportedKey["controller"] = m_controller;

  if (!m_revoked.empty())
    exportedKey["revoked"] = m_revoked;

  return exportedKey;
}
