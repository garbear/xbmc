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

#include "DIDConstants.h"

using namespace KODI;
using namespace CRYPTO;

// clang-format off
const std::array<std::string, 5> CDIDConstants::VERIFICATION_RELATIONSHIPS{
  "assertionMethod",
  "authentication",
  "capabilityDelegation",
  "capabilityInvocation",
  "keyAgreement",
};
// clang-format on
