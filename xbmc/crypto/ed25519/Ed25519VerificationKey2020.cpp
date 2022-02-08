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

#include "Ed25519VerificationKey2020.h"

#include "OpenSSLEd25519Provider.h"
#include "crypto/CryptoProvider.h"
#include "crypto/codecs/Base58BTC.h"
#include "crypto/cryptold/LDTranslator.h"
#include "crypto/random/OpenSSLRandomGenerator.h"
#include "utils/log.h"

using namespace KODI;
using namespace CRYPTO;

namespace
{
// Multibase base58-btc header
constexpr char MULTIBASE_BASE58BTC_HEADER = 'z';

// Multicodec ed25519-pub header
const ByteArray MULTICODEC_ED25519_PUB_HEADER = {0xed, 0x01};

// Multicodec ed25519-priv header
const ByteArray MULTICODEC_ED25519_PRIV_HEADER = {0x80, 0x26};
} // namespace

const std::string CEd25519VerificationKey2020::SUITE_ID = "Ed25519VerificationKey2020";
const std::string CEd25519VerificationKey2020::SUITE_CONTEXT =
    "https://w3id.org/security/suites/ed25519-2020/v1";

CEd25519VerificationKey2020::CEd25519VerificationKey2020(std::string strId,
                                                         std::string controller,
                                                         std::string revoked,
                                                         std::string publicKeyMultibase,
                                                         std::string privateKeyMultibase)
  : CLDKeyPair(std::move(strId), std::move(controller), std::move(revoked)),
    m_publicKeyMultibase(std::move(publicKeyMultibase)),
    m_privateKeyMultibase(std::move(privateKeyMultibase))
{
  // Log if public key multibase header is invalid
  if (!m_publicKeyMultibase.empty() &&
      !IsValidKeyHeader(m_publicKeyMultibase, MULTICODEC_ED25519_PUB_HEADER))
  {
    CLog::Log(LOGERROR, "Invalid multibase public key: {}", m_publicKeyMultibase);
  }

  // Log if private key multibase header is invalid
  if (!m_privateKeyMultibase.empty() &&
      !IsValidKeyHeader(m_privateKeyMultibase, MULTICODEC_ED25519_PRIV_HEADER))
  {
    CLog::Log(LOGERROR, "Invalid multibase private key: {}", m_privateKeyMultibase);
  }

  // Set key identifier if controller is provided
  if (m_id.empty() && !m_controller.empty() && !GetFingerprint().empty())
    m_id = m_controller + "#" + GetFingerprint();
}

CEd25519VerificationKey2020::~CEd25519VerificationKey2020() = default;

LDKeyType CEd25519VerificationKey2020 ::GetType() const
{
  return LDKeyType::Ed25519VerificationKey2020;
}

CVariant CEd25519VerificationKey2020::Export(bool publicKey,
                                             bool privateKey,
                                             bool includeContext) const
{
  CVariant exportedKey = CLDKeyPair::Export(publicKey, privateKey, includeContext);

  if (includeContext)
    exportedKey["@context"] = SUITE_CONTEXT;

  if (publicKey)
    exportedKey["publicKeyMultibase"] = m_publicKeyMultibase;

  if (privateKey)
    exportedKey["privateKeyMultibase"] = m_privateKeyMultibase;

  return exportedKey;
}

bool CEd25519VerificationKey2020::VerifyFingerprint(const std::string& fingerprint) const
{
  // Fingerprint should have multibase base58-btc header
  if (fingerprint.empty() || fingerprint[0] != MULTIBASE_BASE58BTC_HEADER)
    return false;

  // Decode base58-btc data
  ByteArray fingerprintBuffer;
  if (!CBase58BTC::DecodeBase58(fingerprint.substr(1), fingerprintBuffer))
    return false;

  // Check for multicodec header
  if (fingerprintBuffer.size() < 2 || fingerprintBuffer[0] != MULTICODEC_ED25519_PUB_HEADER[0] ||
      fingerprintBuffer[1] != MULTICODEC_ED25519_PUB_HEADER[1])
    return false;

  ByteArray publicKeyBuffer = GetPublicKeyBuffer();
  if (fingerprintBuffer.size() - 2 != publicKeyBuffer.size())
    return false;

  // Remove multicodec header for comparison
  fingerprintBuffer.erase(fingerprintBuffer.begin(),
                          fingerprintBuffer.begin() + MULTICODEC_ED25519_PUB_HEADER.size());

  return fingerprintBuffer == publicKeyBuffer;
}

std::unique_ptr<CLDKeyPair> CEd25519VerificationKey2020::From(const CVariant& keyPair)
{
  std::string strId = keyPair["id"].asString();
  std::string controller = keyPair["controller"].asString();
  std::string revoked = keyPair["revoked"].asString();
  std::string publicKeyMultibase = keyPair["publicKeyMultibase"].asString();
  std::string privateKeyMultibase = keyPair["privateKeyMultibase"].asString();

  return std::make_unique<CEd25519VerificationKey2020>(
      std::move(strId), std::move(controller), std::move(revoked), std::move(publicKeyMultibase),
      std::move(privateKeyMultibase));
}

std::unique_ptr<CLDKeyPair> CEd25519VerificationKey2020::Generate(std::string strId,
                                                                  std::string controller,
                                                                  std::string revoked)
{
  // Create Ed25519 signature scheme
  std::unique_ptr<IEd25519Provider> ed25519Provider(new COpenSSLEd25519Provider);
  std::unique_ptr<IRandomGenerator> csrng(new COpenSSLRandomGenerator);

  // Create crypto provider
  CCryptoProvider cryptoProvider(std::move(ed25519Provider), std::move(csrng));

  // Generate public/private key pair of the main did:key signing/verification key
  const KeyPair keyPair = cryptoProvider.GenerateKeys(CRYPTO::Key::Type::Ed25519);

  std::string publicKeyMultibase = EncodeMultibasePublicKey(keyPair.publicKey);
  std::string privateKeyMultibase = EncodeMultibasePrivateKey(keyPair.privateKey);

  return std::make_unique<CEd25519VerificationKey2020>(
      std::move(strId), std::move(controller), std::move(revoked), std::move(publicKeyMultibase),
      std::move(privateKeyMultibase));
}

std::unique_ptr<CLDKeyPair> CEd25519VerificationKey2020::FromFingerprint(std::string fingerprint)
{
  return std::make_unique<CEd25519VerificationKey2020>("", "", "", std::move(fingerprint), "");
}

ByteArray CEd25519VerificationKey2020::GetPublicKeyBuffer() const
{
  ByteArray result;

  if (!m_publicKeyMultibase.empty())
  {
    // Remove multibase header and decode
    if (CBase58BTC::DecodeBase58(m_publicKeyMultibase.substr(1), result))
    {
      // Remove multicodec header
      result.erase(result.begin(), result.begin() + MULTICODEC_ED25519_PUB_HEADER.size());
    }
  }

  return result;
}

bool CEd25519VerificationKey2020::IsValidKeyHeader(const std::string& multibaseKey,
                                                   const ByteArray& expectedHeader)
{
  if (multibaseKey.empty())
    return false;

  if (multibaseKey[0] != MULTIBASE_BASE58BTC_HEADER)
    return false;

  ByteArray keyBytes;
  if (!CBase58BTC::DecodeBase58(multibaseKey.substr(1), keyBytes))
    return false;

  for (size_t i = 0; i < expectedHeader.size(); ++i)
  {
    if (expectedHeader[i] != keyBytes[i])
      return false;
  }

  return true;
}

std::string CEd25519VerificationKey2020::EncodeMultibasePublicKey(const PublicKey& key)
{
  return EncodeMultibaseKey(MULTICODEC_ED25519_PUB_HEADER, key.data);
}

std::string CEd25519VerificationKey2020::EncodeMultibasePrivateKey(const PrivateKey& key)
{
  return EncodeMultibaseKey(MULTICODEC_ED25519_PRIV_HEADER, key.data);
}

std::string CEd25519VerificationKey2020::EncodeMultibaseKey(const ByteArray& header,
                                                            const ByteArray& key)
{
  ByteArray multibaseKey;
  multibaseKey.reserve(header.size() + key.size());

  multibaseKey.insert(multibaseKey.end(), header.begin(), header.end());
  multibaseKey.insert(multibaseKey.end(), key.begin(), key.end());

  return MULTIBASE_BASE58BTC_HEADER + CBase58BTC::EncodeBase58(multibaseKey);
}
