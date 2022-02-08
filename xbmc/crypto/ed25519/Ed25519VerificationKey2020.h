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

#pragma once

#include "crypto/CryptoTypes.h"
#include "crypto/cryptold/LDKeyPair.h"

#include <memory>
#include <string>

namespace KODI
{
namespace CRYPTO
{
struct PublicKey;
struct PrivateKey;

/*!
 * \brief An implementation of the Ed25519VerificationKey2020 spec, for use
 *        with Linked Data Proofs
 *
 * See also:
 *
 *   - https://w3c-ccg.github.io/lds-ed25519-2020/#ed25519verificationkey2020
 *   - https://github.com/digitalbazaar/jsonld-signatures
 */
class CEd25519VerificationKey2020 : public CLDKeyPair
{
public:
  // JSON-LD identifier for key suite
  static const std::string SUITE_ID;

  // JSON-LD context for key suite
  static const std::string SUITE_CONTEXT;

  /*!
   * \brief Create a key object
   *
   * \param strId The key ID. If empty, the ID will be composed of controller
   *              and the key fingerprint as the hash fragment.
   * \param controller Controller DID or document URL
   * \param revoked Timestamp of when the key has been revoked, in RFC3339 format
   * \param publicKeyMultibase Multibase-encoded public key with a multicodec
   *                           ed25519-pub varint header [0xed, 0x01]
   * \param privateKeyMultibase Multibase-encoded private key with a multicodec
   *                            ed25519-priv varint header [0x80, 0x26]
   *
   * If `revoked` is empty, the key itself is considered not revoked. Note that
   * this mechanism is slightly different than DID Document key revocation, where
   * a DID controller can revoke a key from that DID by removing it from the DID
   * Document.
   */
  CEd25519VerificationKey2020(std::string strId,
                              std::string controller,
                              std::string revoked,
                              std::string publicKeyMultibase,
                              std::string privateKeyMultibase);

  ~CEd25519VerificationKey2020() override;

  // Implementation of LDKeyPair
  LDKeyType GetType() const override;
  CVariant Export(bool publicKey, bool privateKey, bool includeContext) const override;
  std::string GetFingerprint() const override { return m_publicKeyMultibase; }
  bool VerifyFingerprint(const std::string& fingerprint) const override;

  // Properties
  const std::string& GetPublicKeyMultibase() const { return m_publicKeyMultibase; }
  const std::string& GetPrivateKeyMultibase() const { return m_privateKeyMultibase; }

  /**
   * \brief Creates an Ed25519 Key Pair from an existing serialized key pair
   *
   * \param keyPair The existing serialized key pair
   *
   * \return An Ed25519 Key Pair
   */
  static std::unique_ptr<CLDKeyPair> From(const CVariant& keyPair);

  /*!
   * \brief Generates a key pair
   *
   * \return The generated public/private key pair
   */
  static std::unique_ptr<CLDKeyPair> Generate(std::string strId,
                                              std::string controller,
                                              std::string revoked);

  /*!
   * \brief Creates an instance of Ed25519VerificationKey2020 from a key
   *        fingerprint
   *
   * \param fingerprint The multibase-encoded key fingerprint
   *
   * \return The key pair instance (with public key only)
   */
  static std::unique_ptr<CLDKeyPair> FromFingerprint(std::string fingerprint);

private:
  /*!
   * \brief Get the public key as an array of bytes
   */
  ByteArray GetPublicKeyBuffer() const;

  /*!
   * \brief Check a multibase key for an expected header
   */
  static bool IsValidKeyHeader(const std::string& multibaseKey, const ByteArray& expectedHeader);

  /*!
   * \brief Encode a multibase base58-btc multicodec public key
   */
  static std::string EncodeMultibasePublicKey(const PublicKey& key);

  /*!
   * \brief Encode a multibase base58-btc multicodec private key
   */
  static std::string EncodeMultibasePrivateKey(const PrivateKey& key);

  /*!
   * \brief Encode a multibase base58-btc multicodec public or private key
   */
  static std::string EncodeMultibaseKey(const ByteArray& header, const ByteArray& key);

  std::string m_publicKeyMultibase;
  std::string m_privateKeyMultibase;
};
} // namespace CRYPTO
} // namespace KODI
