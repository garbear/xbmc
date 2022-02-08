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

#pragma once

#include "utils/Variant.h"

#include <memory>
#include <string>

namespace KODI
{
namespace CRYPTO
{
class CLDKeyPair;

/*!
 * \brief DID "key" method
 *
 * The did:key method is used to express public keys in a way that doesn't
 * require a DID Registry of any kind. Its general format is:
 *
 *   did:key:<multibase encoded, multicodec identified, public key>
 *
 * So, for example, the following DID would be derived from a base-58 encoded
 * ed25519 public key:
 *
 *   did:key:z6MkpTHR8VNsBxYAAWHut2Geadd9jSwuBV8xRoAnwWsdvktH
 *
 * That DID would correspond to the following DID document:
 *
 *   {
 *     "@context": [
 *       "https://www.w3.org/ns/did/v1",
 *       "https://w3id.org/security/suites/ed25519-2020/v1",
 *       "https://w3id.org/security/suites/x25519-2020/v1"
 *     ],
 *     "id": "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK",
 *     "verificationMethod": [{
 *       "id": "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK",
 *       "type": "Ed25519VerificationKey2020",
 *       "controller": "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK",
 *       "publicKeyMultibase": "z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"
 *     }],
 *     "authentication": [
 *       "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"
 *     ],
 *     "assertionMethod": [
 *       "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"
 *     ],
 *     "capabilityDelegation": [
 *       "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"
 *     ],
 *     "capabilityInvocation": [
 *       "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK"
 *     ],
 *     "keyAgreement": [{
 *       "id": "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK#z6LSj72tK8brWgZja8NLRwPigth2T9QRiG1uH9oKZuKjdh9p",
 *       "type": "X25519KeyAgreementKey2020",
 *       "controller": "did:key:z6MkhaXgBZDvotDkL5257faiztiGiC2QtKLGpbnnEGta2doK",
 *       "publicKeyMultibase": "z6LSj72tK8brWgZja8NLRwPigth2T9QRiG1uH9oKZuKjdh9p"
 *     }]
 *   }
 *
 * @todo The constructed document is missing the X25519 context and the
 *       "keyAgreement" property. Need to use libsodium to convert from
 *       Ed25519 to X25519 so we can add this later.
 */
class CDIDKeyDriver
{
public:
  static const std::string METHOD;

  /*!
   * \brief Generate a new key and get its corresponding did:key method DID
   *        Document
   *
   * Resolves with the generated DID Document, along with the corresponding
   * key pairs used to generate it (for storage in a KMS).
   *
   * \param[out] didDocument The DID Document
   * \param[out] keyPair The key pair
   */
  static void Generate(CVariant& didDocument, std::unique_ptr<CLDKeyPair>& keyPair);

  /*!
   * \brief Converts a public key object to a `did:key` method DID Document
   *
   * Note that unlike `Generate()`, a key pair is not returned. Use
   * `PublicMethodFor()` to fetch keys for particular proof purposes.
   *
   * \param publicKeyDescription Public key object used to generate the DID
   *
   * \return The generated DID Document
   */
  static CVariant PublicKeyToDidDocument(const CLDKeyPair& publicKeyDescription);

  /*!
   * \brief Get a full DID Document from a did:key DID
   *
   * \return The DID Document from the existing did:key DID
   */
  static CVariant Get(const std::string& did);

  /*!
   * \brief Often, you have just a did:key DID, and you need to get a key for
   * a particular purpose from it, such as an assertionMethod key to verify a
   * VC signature, or a keyAgreement key to encrypt a document for that DID's
   * controller.
   *
   * \param didDocument The DID Document
   * \param purpose The given purpose (authentication, assertionMethod,
   *        keyAgreement, etc).
   *
   * \return The key agreement data, assertion method data, etc.
   */
  static CVariant PublicMethodFor(const CVariant& didDocument, const std::string& purpose);

  /*!
   * \brief Computes and returns the ID of a given key pair
   *
   * \param keyPair The key pair used when computing the identifier
   *
   * \return The key's ID
   */
  static std::string ComputeID(const CLDKeyPair& keyPair);

private:
  /*!
   * \brief Converts an Ed25519KeyPair object to a did:key method DID Document
   *
   * \param keyPair Key used to generate the DID document
   *
   * \return The generated DID Document
   */
  static CVariant KeyPairToDidDocument(const CLDKeyPair& keyPair);

  /*!
   * \brief Returns the public key object for a given key ID fragment
   *
   * \param didDocument The DID Document to use when generating the ID
   * \param keyIdFragment The key identifier fragment
   *
   * \return The public key node, with `@context`
   */
  static CVariant GetKey(const CVariant& didDocument, const std::string& keyIdFragment);
};
} // namespace CRYPTO
} // namespace KODI
