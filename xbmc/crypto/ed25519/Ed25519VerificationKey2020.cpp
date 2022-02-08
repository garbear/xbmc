/*
 *  Copyright (C) 2022-2026 Team Kodi
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
#include "crypto/multiformats/Multibase.h"
#include "crypto/multiformats/Multicodec.h"
#include "crypto/random/OpenSSLRandomGenerator.h"
#include "utils/log.h"

using namespace KODI;
using namespace CRYPTO;

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
      !IsValidKeyHeader(m_publicKeyMultibase, CMulticodec::Code::ED25519_PUB))
  {
    CLog::Log(LOGERROR, "Invalid multibase public key: {}", m_publicKeyMultibase);
  }

  // Log if private key multibase header is invalid
  if (!m_privateKeyMultibase.empty() &&
      !IsValidKeyHeader(m_privateKeyMultibase, CMulticodec::Code::ED25519_PRIV))
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
  if (fingerprint.empty() || fingerprint[0] != static_cast<char>(CMultibase::TranslateEncoding(
                                                   CMultibase::Encoding::BASE58BTC)))
    return false;

  // Decode base58-btc data
  std::vector<uint8_t> fingerprintBuffer;
  if (!CBase58BTC::DecodeBase58(fingerprint.substr(1), fingerprintBuffer))
    return false;

  CMulticodec::Code code = CMulticodec::Code::UNKNOWN;
  size_t headerSize = 0;
  if (!CMulticodec::Decode(fingerprintBuffer.data(), fingerprintBuffer.size(), code, headerSize) ||
      code != CMulticodec::Code::ED25519_PUB)
    return false;

  std::vector<uint8_t> publicKeyBuffer = GetPublicKeyBuffer();
  if (fingerprintBuffer.size() - headerSize != publicKeyBuffer.size())
    return false;

  // Remove multicodec header for comparison
  fingerprintBuffer.erase(fingerprintBuffer.begin(), fingerprintBuffer.begin() + headerSize);

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

std::vector<uint8_t> CEd25519VerificationKey2020::GetPublicKeyBuffer() const
{
  std::vector<uint8_t> result;

  if (!m_publicKeyMultibase.empty())
  {
    // Remove multibase header and decode
    if (CBase58BTC::DecodeBase58(m_publicKeyMultibase.substr(1), result))
    {
      CMulticodec::Code code = CMulticodec::Code::UNKNOWN;
      size_t headerSize = 0;
      if (!CMulticodec::Decode(result.data(), result.size(), code, headerSize) ||
          code != CMulticodec::Code::ED25519_PUB)
      {
        return {};
      }

      result.erase(result.begin(), result.begin() + headerSize);
    }
  }

  return result;
}

bool CEd25519VerificationKey2020::IsValidKeyHeader(const std::string& multibaseKey,
                                                   CMulticodec::Code expectedCode)
{
  if (multibaseKey.empty())
    return false;

  if (multibaseKey[0] !=
      static_cast<char>(CMultibase::TranslateEncoding(CMultibase::Encoding::BASE58BTC)))
    return false;

  std::vector<uint8_t> keyBytes;
  if (!CBase58BTC::DecodeBase58(multibaseKey.substr(1), keyBytes))
    return false;

  CMulticodec::Code code = CMulticodec::Code::UNKNOWN;
  size_t headerSize = 0;
  return CMulticodec::Decode(keyBytes.data(), keyBytes.size(), code, headerSize) &&
         code == expectedCode;
}

std::string CEd25519VerificationKey2020::EncodeMultibasePublicKey(const PublicKey& key)
{
  return EncodeMultibaseKey(CMulticodec::Code::ED25519_PUB, key.data);
}

std::string CEd25519VerificationKey2020::EncodeMultibasePrivateKey(const PrivateKey& key)
{
  return EncodeMultibaseKey(CMulticodec::Code::ED25519_PRIV, key.data);
}

std::string CEd25519VerificationKey2020::EncodeMultibaseKey(CMulticodec::Code code,
                                                            const std::vector<uint8_t>& key)
{
  std::vector<uint8_t> multibaseKey;
  if (!CMulticodec::EncodePrefix(code, key.data(), key.size(), multibaseKey))
    return {};

  return static_cast<char>(CMultibase::TranslateEncoding(CMultibase::Encoding::BASE58BTC)) +
         CBase58BTC::EncodeBase58(multibaseKey.data(), multibaseKey.size());
}
