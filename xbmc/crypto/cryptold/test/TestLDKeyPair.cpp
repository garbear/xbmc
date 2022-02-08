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

#include "crypto/cryptold/LDKeyPair.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestLDKeyPair, Constructor)
{
  //
  // Spec: Should initialize ID and controller
  //

  const std::string strId = "did:ex:1234#fingerprint";
  const std::string controller = "did:ex:1234";
  const CLDKeyPair keyPair(strId, controller, "");

  EXPECT_EQ(keyPair.GetID(), strId);
  EXPECT_EQ(keyPair.GetController(), controller);
}

TEST(TestLDKeyPair, Export)
{
  const std::string strId = "did:ex:1234#fingerprint";
  const std::string controller = "did:ex:1234";
  const CLDKeyPair keyPair(strId, controller, "");

  CVariant exportedKey = keyPair.Export(true, true, true);

  EXPECT_EQ(exportedKey["id"].asString(), strId);
  EXPECT_EQ(exportedKey["controller"].asString(), controller);
  EXPECT_TRUE(exportedKey["revoked"].empty());
}
