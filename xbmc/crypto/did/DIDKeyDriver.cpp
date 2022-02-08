/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the did-method-key project under the BSD
 *  3-Clause License - https://github.com/digitalbazaar/did-method-key
 *  Copyright (C) 2018-2021 Digital Bazaar, Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

#include "DIDKeyDriver.h"

#include "DIDConstants.h"
#include "crypto/cryptold/LDKeyPair.h"
#include "crypto/cryptold/LDTranslator.h"
#include "crypto/cryptold/LDUtils.h"
#include "crypto/ed25519/Ed25519VerificationKey2020.h"
#include "utils/StringUtils.h"

#include <cstring>
#include <map>
#include <vector>

using namespace KODI;
using namespace CRYPTO;

namespace
{
// JSON-LD context
constexpr const char* DID_CONTEXT_URL = "https://www.w3.org/ns/did/v1";
} // namespace

const std::string CDIDKeyDriver::METHOD = "key";

void CDIDKeyDriver::Generate(CVariant& didDocument, std::unique_ptr<CLDKeyPair>& keyPair)
{
  // Generate public/private key pair of the main did:key signing/verification key
  std::unique_ptr<CLDKeyPair> verificationKeyPair =
      CEd25519VerificationKey2020::Generate("", "", "");

  didDocument = KeyPairToDidDocument(*verificationKeyPair);

  keyPair = std::move(verificationKeyPair);
}

CVariant CDIDKeyDriver::PublicKeyToDidDocument(const CLDKeyPair& publicKey)
{
  return KeyPairToDidDocument(publicKey);
}

CVariant CDIDKeyDriver::Get(const std::string& did)
{
  std::string didAuthority;
  std::string keyIdFragment;

  std::vector<std::string> parts = StringUtils::Split(did, '#', 2);

  if (parts.size() == 2)
  {
    didAuthority = std::move(parts[0]);
    keyIdFragment = std::move(parts[1]);
  }
  else
  {
    didAuthority = did;
  }

  const std::string fingerprint = didAuthority.substr(std::strlen("did:key:"));
  std::unique_ptr<CLDKeyPair> keyPair = CEd25519VerificationKey2020::FromFingerprint(fingerprint);
  keyPair->SetController(didAuthority);

  CVariant didDocument = KeyPairToDidDocument(*keyPair);

  if (!keyIdFragment.empty())
  {
    // Resolve an individual key
    didDocument = GetKey(didDocument, keyIdFragment);
  }

  // Resolve the full DID Document
  return didDocument;
}

CVariant CDIDKeyDriver::PublicMethodFor(const CVariant& didDocument, const std::string& purpose)
{
  return CLDUtils::FindVerificationMethod(didDocument, purpose);
}

std::string CDIDKeyDriver::ComputeID(const CLDKeyPair& keyPair)
{
  return "did:key:" + keyPair.GetFingerprint() + "#" + keyPair.GetFingerprint();
}

CVariant CDIDKeyDriver::KeyPairToDidDocument(const CLDKeyPair& keyPair)
{
  CVariant didDocument;

  // Generate list of JSON-LD contexts used by this document
  CVariant contexts{CVariant::VariantTypeArray};
  contexts.push_back(DID_CONTEXT_URL);
  contexts.push_back(CEd25519VerificationKey2020::SUITE_CONTEXT);

  CVariant verificationKeyPair = keyPair.Export(true, false, false);

  // Get the DID
  const std::string did = "did:key:" + keyPair.GetFingerprint();

  // Set the controller
  verificationKeyPair["controller"] = did;

  // Now set the source key's ID
  const std::string keyId = did + "#" + keyPair.GetFingerprint();
  verificationKeyPair["id"] = keyId;

  // Compose the DID Document
  didDocument = CVariant{CVariant::VariantTypeObject};
  // Note that did:key does not have its own method-specific context,
  // and only uses the general DID Core context, and key-specific contexts
  didDocument["@context"] = contexts;
  didDocument["id"] = did;
  didDocument["verificationMethod"].push_back(std::move(verificationKeyPair));
  didDocument["authentication"].push_back(keyId);
  didDocument["assertionMethod"].push_back(keyId);
  didDocument["capabilityDelegation"].push_back(keyId);
  didDocument["capabilityInvocation"].push_back(keyId);

  return didDocument;
}

CVariant CDIDKeyDriver::GetKey(const CVariant& didDocument, const std::string& keyIdFragment)
{
  // Determine if the key ID fragment belongs to the "main" public key
  const std::string keyId = didDocument["id"].asString() + '#' + keyIdFragment;

  const CVariant& verificationMethods = didDocument["verificationMethod"];

  for (auto it = verificationMethods.begin_array(); it != verificationMethods.end_array(); ++it)
  {
    CVariant publicKey = *it;

    if (publicKey["id"].asString() == keyId)
    {
      // Set the context
      LDKeyType keyType = CLDTranslator::TranslateKeyType(publicKey["type"].asString());
      std::string suiteContext = CLDTranslator::GetLDContextBySuite(keyType);
      if (!suiteContext.empty())
        publicKey["@context"] = std::move(suiteContext);

      return publicKey;
    }
  }

  return CVariant{};
}
