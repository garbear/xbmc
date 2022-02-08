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

#pragma once

#include "LDTypes.h"
#include "utils/Variant.h"

#include <string>

namespace KODI
{
namespace CRYPTO
{

/*!
 * \brief Linked Data key pair
 *
 * When adding support for a new suite type, developers should do the following:
 *
 * 1. Subclass CLDKeyPair
 * 2. Override relevant methods (such as `Export()` and `GetFingerprint()`)
 * 3. Add the key type to the table in the LDTypes.h header
 */
class CLDKeyPair
{
public:
  /*!
   * \brief Creates a public/private key pair instance
   *
   * This is an abstract base class, actual key material and suite-specific
   * methods are handled in the subclass.
   *
   * To generate or import a key pair, use the `CCryptoLD` instance. See
   * CryptoLD.h.
   *
   * \param strId The key ID, typically composed of controller URL and key
   *              fingerprint as hash fragment
   * \param controller The DID/URL of the person/entity controlling this key
   * \param revoked Timestamp of when the key has been revoked, in RFC3339
   *                format
   *
   * If `revoked` is not present, the key itself is considered not revoked.
   *
   * Note that this mechanism is slightly different than DID Document key
   * revocation, where a DID controller can revoke a key from that DID by
   * removing it from the DID Document.
   */
  CLDKeyPair(std::string strId, std::string controller, std::string revoked);

  virtual ~CLDKeyPair();

  // Accessors
  const std::string& GetID() const { return m_id; }
  const std::string& GetController() const { return m_controller; }
  const std::string& GetRevoked() const { return m_revoked; }

  // Mutators
  void SetID(const std::string& strId) { m_id = strId; }
  void SetController(const std::string& controller) { m_controller = controller; }
  void SetRevoked(const std::string& revoked) { m_revoked = revoked; }

  /*!
   * \brief Get the key type from the subclass
   */
  virtual LDKeyType GetType() const { return LDKeyType::UNKNOWN; }

  /*!
   * \brief Exports the serialized representation of the KeyPair and other
   *        information that json-ld Signatures can use to form a proof
   *
   * NOTE: Subclasses MUST override this method (and add the exporting of
   * their public and private key material).
   *
   * \param publicKey True to export public key material, false to omit
   * \param privateKey True to export private key material, false to omit
   * \param includeContext True to include the JSON-LD @context, false to omit
   *
   * \return A public key object information used in verification methods
   *         by signatures
   */
  virtual CVariant Export(bool publicKey, bool privateKey, bool includeContext) const;

  /*!
   * \brief Returns the public key fingerprint, multibase+multicodec encoded
   *
   * The specific fingerprint method is determined by the key suite, and is
   * often either a hash of the public key material (such as with RSA), or the
   * full encoded public key (for key types with sufficiently short
   * representations, such as ed25519).
   *
   * This is frequently used in initializing the key id, or generating some
   * types of cryptonym DIDs.
   *
   * \return The multibase+multicodec-encoded fingerprint, or empty if unknown
   */
  virtual std::string GetFingerprint() const { return ""; }

  /*!
   * \brief Tests whether the fingerprint was generated from a given key pair
   *
   * \param fingerprint A public key fingerprint
   *
   * \return True if verification succeeded, false if verification failed
   */
  virtual bool VerifyFingerprint(const std::string& fingerprint) const { return false; }

protected:
  // Construction parameters
  std::string m_id;
  std::string m_controller;
  std::string m_revoked;
};

} // namespace CRYPTO
} // namespace KODI
