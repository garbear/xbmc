/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the did-method-key project under the BSD
 *  3-Clause License - https://github.com/digitalbazaar/did-method-key
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#include "crypto/cryptold/LDKeyPair.h"
#include "crypto/did/DIDKeyDriver.h"
#include "crypto/ed25519/Ed25519VerificationKey2020.h"
#include "utils/Variant.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestDIDKeyDriver, Get)
{
  //
  // Spec: Should get the DID Document for a did:key DID
  //

  const std::string did = "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";
  const std::string keyId = "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T"
                            "#z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";

  const CVariant didDocument = CDIDKeyDriver::Get(did);

  EXPECT_EQ(didDocument["id"].asString(), did);

  EXPECT_EQ(didDocument["@context"].size(), 2);
  EXPECT_EQ(didDocument["@context"][0].asString(), "https://www.w3.org/ns/did/v1");
  EXPECT_EQ(didDocument["@context"][1].asString(),
            "https://w3id.org/security/suites/ed25519-2020/v1");

  EXPECT_EQ(didDocument["authentication"][0].asString(), keyId);
  EXPECT_EQ(didDocument["assertionMethod"][0].asString(), keyId);
  EXPECT_EQ(didDocument["capabilityDelegation"][0].asString(), keyId);
  EXPECT_EQ(didDocument["capabilityInvocation"][0].asString(), keyId);

  const CVariant publicKey = didDocument["verificationMethod"][0];
  EXPECT_EQ(publicKey["id"].asString(), keyId);
  EXPECT_EQ(publicKey["type"].asString(), "Ed25519VerificationKey2020");
  EXPECT_EQ(publicKey["controller"].asString(), did);
  EXPECT_EQ(publicKey["publicKeyMultibase"].asString(),
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");

  //
  // Spec: Should resolve an individual key within the DID Document
  //

  const CVariant key = CDIDKeyDriver::Get(keyId);

  EXPECT_EQ(key["@context"].asString(), "https://w3id.org/security/suites/ed25519-2020/v1");
  EXPECT_EQ(key["id"].asString(), "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
                                  "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(key["type"].asString(), "Ed25519VerificationKey2020");
  EXPECT_EQ(key["controller"].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(key["publicKeyMultibase"].asString(),
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
}

TEST(TestDIDKeyDriver, Generate)
{
  //
  // Spec: Should generate and get round trip
  //

  CVariant didDocument;
  std::unique_ptr<CLDKeyPair> keyPair;
  CDIDKeyDriver::Generate(didDocument, keyPair);

  const std::string did = didDocument["id"].asString();
  const std::string keyId = didDocument["authentication"][0].asString();

  const CVariant& verificationKeyPair = didDocument["verificationMethod"][0];

  EXPECT_EQ(verificationKeyPair["id"].asString(), keyId);
  EXPECT_EQ(verificationKeyPair["controller"].asString(), did);
}

TEST(TestDIDKeyDriver, PublicKeyToDidDocument)
{
  //
  // Spec: Should convert a key pair instance into a DID Document
  //

  // Note that a freshly-generated key pair does not have a controller or
  // key ID
  const std::unique_ptr<CLDKeyPair> keyPair = CEd25519VerificationKey2020::Generate("", "", "");

  CVariant didDocument = CDIDKeyDriver::PublicKeyToDidDocument(*keyPair);

  EXPECT_TRUE(!didDocument.empty());
  EXPECT_TRUE(didDocument["@context"].isString() || didDocument["@context"].isArray());
  EXPECT_EQ(didDocument["id"].asString(), "did:key:" + keyPair->GetFingerprint());

  //
  // Spec: Should convert a plain object to a DID Document
  //

  CVariant publicKeyDescription{CVariant::VariantTypeObject};

  publicKeyDescription["@context"] = "https://w3id.org/security/suites/ed25519-2020/v1";
  publicKeyDescription["id"] = "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T"
                               "#z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";
  publicKeyDescription["type"] = "Ed25519VerificationKey2020";
  publicKeyDescription["controller"] = "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";
  publicKeyDescription["publicKeyMultibase"] = "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";

  std::unique_ptr<CLDKeyPair> publicKey = CEd25519VerificationKey2020::From(publicKeyDescription);

  didDocument = CDIDKeyDriver::PublicKeyToDidDocument(*publicKey);

  EXPECT_TRUE(!didDocument.empty());
  EXPECT_EQ(didDocument["@context"][0].asString(), "https://www.w3.org/ns/did/v1");
  EXPECT_EQ(didDocument["@context"][1].asString(),
            "https://w3id.org/security/suites/ed25519-2020/v1");
  EXPECT_EQ(didDocument["id"].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(didDocument["authentication"][0].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(didDocument["assertionMethod"][0].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(didDocument["capabilityDelegation"][0].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(didDocument["capabilityInvocation"][0].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");

  const CVariant& verificationMethod = didDocument["verificationMethod"][0];

  EXPECT_EQ(verificationMethod["id"].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(verificationMethod["controller"].asString(),
            "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
  EXPECT_EQ(verificationMethod["type"].asString(), "Ed25519VerificationKey2020");
  EXPECT_EQ(verificationMethod["publicKeyMultibase"].asString(),
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
}

TEST(TestDIDKeyDriver, PublicMethodFor)
{
  //
  // Spec: Should find a key for a DID Document and purpose
  //

  const std::string did = "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T";

  // First, get the DID Document
  const CVariant didDocument = CDIDKeyDriver::Get(did);

  // Then PublicMethodFor() can be used to fetch key data
  const CVariant authKeyData = CDIDKeyDriver::PublicMethodFor(didDocument, "authentication");

  EXPECT_TRUE(authKeyData.isObject());

  EXPECT_EQ(authKeyData["type"].asString(), "Ed25519VerificationKey2020");
  EXPECT_EQ(authKeyData["publicKeyMultibase"].asString(),
            "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");

  //
  // Spec: Should fail if key is not found for purpose
  //

  const CVariant emptyData = CDIDKeyDriver::PublicMethodFor(didDocument, "invalidPurpose");
  EXPECT_TRUE(emptyData.isNull());
}

TEST(TestDIDKeyDriver, ComputeID)
{
  //
  // Spec: Should set the key ID based on fingerprint
  //

  CEd25519VerificationKey2020 keyPair("", "", "",
                                      "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T", "");

  const std::string keyPairId = CDIDKeyDriver::ComputeID(keyPair);

  EXPECT_EQ(keyPairId, "did:key:z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T#"
                       "z6MknCCLeeHBUaHu4aHSVLDCYQW9gjVJ7a63FpMvtuVMy53T");
}

TEST(TestDIDKeyDriver, Method)
{
  //
  // Spec: Should return DID method ID
  //

  EXPECT_EQ(CDIDKeyDriver::METHOD, "key");
}
