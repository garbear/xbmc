/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the ed25519-verification-key-2020 project under
 *  the BSD 3-Clause License - https://github.com/digitalbazaar/ed25519-verification-key-2020
 *  Copyright (C) 2021 Digital Bazaar, Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#include "crypto/codecs/Base58BTC.h"
#include "crypto/cryptold/LDKeyPair.h"
#include "crypto/ed25519/Ed25519VerificationKey2020.h"
#include "utils/Variant.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

namespace
{
// Multibase base58-btc header
constexpr char MULTIBASE_BASE58BTC_HEADER = 'z';

// Utility function
CVariant GetMockKey()
{
  CVariant mockKey{CVariant::VariantTypeObject};

  mockKey["type"] = "Ed25519VerificationKey2020";
  mockKey["publicKeyMultibase"] = "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";

  return mockKey;
}
} // namespace

TEST(TestEd25519VerificationKey2020, Class)
{
  //
  // Spec: Should expose suite and context properties
  //

  EXPECT_EQ(CEd25519VerificationKey2020::SUITE_ID, "Ed25519VerificationKey2020");
  EXPECT_EQ(CEd25519VerificationKey2020::SUITE_CONTEXT,
            "https://w3id.org/security/suites/ed25519-2020/v1");
}

TEST(TestEd25519VerificationKey2020, Constructor)
{
  const CVariant mockKey = GetMockKey();

  //
  // Spec: Should auto-set key->GetID() based on controller
  //

  std::string controller = "did:example:1234";
  std::string publicKeyMultibase = mockKey["publicKeyMultibase"].asString();

  const std::unique_ptr<CLDKeyPair> keyPair = std::make_unique<CEd25519VerificationKey2020>(
      "", std::move(controller), "", std::move(publicKeyMultibase), "");

  EXPECT_EQ(keyPair->GetID(), "did:example:1234#z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
}

TEST(TestEd25519VerificationKey2020, Generate)
{
  //
  // Spec: Should generate a key pair
  //

  std::unique_ptr<CLDKeyPair> ldKeyPair = CEd25519VerificationKey2020::Generate("", "", "");
  CEd25519VerificationKey2020* ed25519KeyPair =
      dynamic_cast<CEd25519VerificationKey2020*>(ldKeyPair.get());

  ASSERT_NE(ed25519KeyPair, nullptr);

  EXPECT_TRUE(!ed25519KeyPair->GetPublicKeyMultibase().empty());
  EXPECT_TRUE(!ed25519KeyPair->GetPrivateKeyMultibase().empty());

  ByteArray publicKeyBytes;
  ByteArray privateKeyBytes;

  EXPECT_TRUE(
      CBase58BTC::DecodeBase58(ed25519KeyPair->GetPublicKeyMultibase().substr(1), publicKeyBytes));
  EXPECT_TRUE(CBase58BTC::DecodeBase58(ed25519KeyPair->GetPrivateKeyMultibase().substr(1),
                                       privateKeyBytes));

  EXPECT_EQ(publicKeyBytes.size(), 34);

  //! @todo This value was 66 in ed25519-verification-key-2020, but Ed25519
  // private keys are 32 bytes, not 64
  EXPECT_EQ(privateKeyBytes.size(), 34);
}

TEST(TestEd25519VerificationKey2020, Export)
{
  const CVariant mockKey = GetMockKey();

  //
  // Spec: Should export ID, type and key material
  //

  const std::unique_ptr<CLDKeyPair> keyPair =
      CEd25519VerificationKey2020::Generate("", "did:example:1234", "2100-01-01T00:00:00Z");

  const CVariant exported = keyPair->Export(true, true, true);

  EXPECT_EQ(exported["@context"].asString(), "https://w3id.org/security/suites/ed25519-2020/v1");
  EXPECT_EQ(exported["id"].asString().length(),
            strlen("did:example:1234#z6Mkpw72M9suPCBv48X2Xj4YKZJH9W7wzEK1aS6JioKSo89C"));
  EXPECT_EQ(exported["type"].asString(), "Ed25519VerificationKey2020");
  EXPECT_EQ(exported["controller"].asString(), "did:example:1234");
  EXPECT_EQ(exported["revoked"].asString(), "2100-01-01T00:00:00Z");
  EXPECT_EQ(exported["publicKeyMultibase"].asString().length(),
            mockKey["publicKeyMultibase"].asString().length());

  // Ed25519 private keys are the same length as public keys (32 bytes)
  EXPECT_EQ(exported["privateKeyMultibase"].asString().length(),
            mockKey["publicKeyMultibase"].asString().length());
}

TEST(TestEd25519VerificationKey2020, FromFingerprint)
{
  //
  // Spec: Should round-trip load keys
  //

  const std::unique_ptr<CLDKeyPair> keyPair = CEd25519VerificationKey2020::Generate("", "", "");
  CEd25519VerificationKey2020* ed25519KeyPair =
      dynamic_cast<CEd25519VerificationKey2020*>(keyPair.get());
  ASSERT_NE(ed25519KeyPair, nullptr);

  const std::string fingerprint = keyPair->GetFingerprint();
  EXPECT_TRUE(!fingerprint.empty());

  const std::unique_ptr<CLDKeyPair> newKey =
      CEd25519VerificationKey2020::FromFingerprint(fingerprint);
  CEd25519VerificationKey2020* ed25519NewKey =
      dynamic_cast<CEd25519VerificationKey2020*>(newKey.get());
  ASSERT_NE(ed25519NewKey, nullptr);

  EXPECT_EQ(ed25519NewKey->GetPublicKeyMultibase(), ed25519KeyPair->GetPublicKeyMultibase());
}

TEST(TestEd25519VerificationKey2020, From)
{
  //
  // Spec: Should round-trip load exported keys
  //

  const std::unique_ptr<CLDKeyPair> keyPair =
      CEd25519VerificationKey2020::Generate("", "did:example:1234", "");
  const CVariant exported = keyPair->Export(true, true, true);

  const std::unique_ptr<CLDKeyPair> imported = CEd25519VerificationKey2020::From(exported);
  const CVariant exported2 = imported->Export(true, true, true);

  EXPECT_EQ(exported, exported2);
}

TEST(TestEd25519VerificationKey2020, Fingerprint)
{
  //
  // Spec: Should create an Ed25519 key fingerprint
  //

  const std::unique_ptr<CLDKeyPair> keyPair = CEd25519VerificationKey2020::Generate("", "", "");

  const std::string fingerprint = keyPair->GetFingerprint();

  EXPECT_EQ(fingerprint.size(), 48);
  EXPECT_EQ(fingerprint.at(0), 'z');

  //! @todo Check if fingerprint is properly multicodec-encoded
}

TEST(TestEd25519VerificationKey2020, VerifyFingerprint)
{
  //
  // Spec: Should verify a valid fingerprint
  //

  const std::unique_ptr<CLDKeyPair> keyPair = CEd25519VerificationKey2020::Generate("", "", "");

  const std::string fingerprint = keyPair->GetFingerprint();

  EXPECT_TRUE(keyPair->VerifyFingerprint(fingerprint));

  //
  // Spec: Should reject an empty string
  //

  EXPECT_FALSE(keyPair->VerifyFingerprint(""));

  //
  // Spec: Should reject an improperly encoded fingerprint
  //

  const std::string fingerprintChopped = fingerprint.substr(0, fingerprint.length() - 1);

  EXPECT_FALSE(keyPair->VerifyFingerprint(fingerprintChopped));

  //
  // Spec: Should reject an invalid fingerprint
  //

  // Reverse the fingerprint after the multibase header
  std::string badFingerprint = fingerprint.substr(0, 1);
  for (int i = fingerprint.size() - 1; i >= 1; --i)
    badFingerprint += fingerprint[i];

  EXPECT_FALSE(keyPair->VerifyFingerprint(badFingerprint));

  //
  // Spec: Should reject an improperly encoded fingerprint
  //

  EXPECT_FALSE(keyPair->VerifyFingerprint("zPUBLICKEYINFO"));
}
